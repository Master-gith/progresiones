/* core.h - Logica de calculo de progresiones (sin dependencias de Casio).
 * Cada paso del procedimiento se guarda como "formula" (ver gfx.h):
 *   \f{a}{b} fraccion, \r{a} raiz, \p{a} parentesis grandes, _ ^ indices. */
#ifndef CORE_H
#define CORE_H

typedef unsigned long long u64;

#define MAXL  14            /* lineas maximas del procedimiento */
#define LINEW 120

typedef struct {
    char l[MAXL][LINEW];
    int  n;
    int  res;               /* indice de la linea del resultado (-1 = ninguna) */
    int  err;               /* 1 si es un mensaje de error                     */
    char cur[LINEW];
    int  cl;
} Out;

/* ---------------------------------------------------------------- */
/*  Utilidades matematicas propias (no dependemos de libm)          */
/* ---------------------------------------------------------------- */
static int c_len(const char *s) { int n = 0; while (s[n]) n++; return n; }

static int c_isint(double x) {
    if (x > 1e9 || x < -1e9) return 0;
    return x == (double)(long)x;
}

static double c_ipow(double b, long n) {
    double r = 1.0;
    int neg = 0;
    if (n < 0) { neg = 1; n = -n; }
    while (n > 0) {
        if (n & 1) r *= b;
        b *= b;
        n >>= 1;
    }
    return neg ? 1.0 / r : r;
}

static double c_ln(double x) {            /* x > 0 */
    int k = 0, i;
    double y, y2, t, s = 0;
    while (x > 1.5)  { x /= 2; k++; }
    while (x < 0.75) { x *= 2; k--; }
    y = (x - 1) / (x + 1); y2 = y * y; t = y;
    for (i = 1; i < 60; i += 2) { s += t / i; t *= y2; }
    return 2 * s + k * 0.69314718055994531;
}

static double c_exp(double x) {
    const double LN2 = 0.69314718055994531;
    double kf = x / LN2, r, t = 1, s = 1;
    int k, i;
    k = (int)(kf < 0 ? kf - 0.5 : kf + 0.5);
    r = x - k * LN2;
    for (i = 1; i < 30; i++) { t *= r / i; s += t; }
    while (k > 0) { s *= 2; k--; }
    while (k < 0) { s /= 2; k++; }
    return s;
}

static double c_sqrt(double x) {
    double g;
    int i;
    if (x <= 0) return 0;
    g = c_exp(c_ln(x) / 2);
    for (i = 0; i < 4; i++) g = 0.5 * (g + x / g);
    return g;
}

/* ---------------------------------------------------------------- */
/*  Numero -> texto  (10 cifras significativas, E si es muy grande) */
/* ---------------------------------------------------------------- */
static void c_putu(char *b, int *p, u64 v) {
    char t[24]; int n = 0;
    if (v == 0) t[n++] = '0';
    while (v > 0) { t[n++] = '0' + (int)(v % 10); v /= 10; }
    while (n > 0) b[(*p)++] = t[--n];
}

static void c_fmt(double x, char *b) {
    int p = 0, i;
    if (x != x) { b[0]='N'; b[1]='a'; b[2]='N'; b[3]=0; return; }
    if (x > 1e300 || x < -1e300) {
        if (x < 0) b[p++] = '-';
        b[p++]='I'; b[p++]='n'; b[p++]='f'; b[p]=0; return;
    }
    if (x == 0) { b[0] = '0'; b[1] = 0; return; }
    if (x < 0) { b[p++] = '-'; x = -x; }

    if (x >= 1e10 || x < 1e-5) {                 /* notacion cientifica */
        int e = 0; u64 m; char d[8]; int last;
        while (x >= 10) { x /= 10; e++; }
        while (x < 1)   { x *= 10; e--; }
        m = (u64)(x * 1000000.0 + 0.5);
        if (m >= 10000000ULL) { m /= 10; e++; }
        for (i = 6; i >= 0; i--) { d[i] = '0' + (int)(m % 10); m /= 10; }
        last = 6; while (last > 0 && d[last] == '0') last--;
        b[p++] = d[0];
        if (last > 0) { b[p++] = '.'; for (i = 1; i <= last; i++) b[p++] = d[i]; }
        b[p++] = 'E';
        if (e < 0) { b[p++] = '-'; e = -e; }
        c_putu(b, &p, (u64)e);
        b[p] = 0;
        return;
    }
    {
        u64 ip = (u64)x, sc = 1, f, t;
        double fr = x - (double)ip;
        int idg = 1, fd;
        t = ip; while (t >= 10) { t /= 10; idg++; }
        fd = 10 - idg; if (fd > 8) fd = 8; if (fd < 0) fd = 0;
        for (i = 0; i < fd; i++) sc *= 10;
        f = (u64)(fr * (double)sc + 0.5);
        if (f >= sc) { ip++; f -= sc; }
        c_putu(b, &p, ip);
        if (fd > 0 && f > 0) {
            char d[12]; int last;
            for (i = fd - 1; i >= 0; i--) { d[i] = '0' + (int)(f % 10); f /= 10; }
            last = fd - 1; while (last >= 0 && d[last] == '0') last--;
            b[p++] = '.';
            for (i = 0; i <= last; i++) b[p++] = d[i];
        }
        b[p] = 0;
    }
}

/* ---------------------------------------------------------------- */
/*  Texto -> numero.  Acepta  -12.5   3/4   -7/2                    */
/* ---------------------------------------------------------------- */
static int c_num(const char *s, int *i, double *val) {
    double ip = 0, fi = 0, fs = 1;
    int got = 0;
    while (s[*i] >= '0' && s[*i] <= '9') { ip = ip * 10 + (s[*i] - '0'); (*i)++; got = 1; }
    if (s[*i] == '.') {
        (*i)++;
        while (s[*i] >= '0' && s[*i] <= '9') { fi = fi * 10 + (s[*i] - '0'); fs *= 10; (*i)++; got = 1; }
    }
    if (!got) return 0;
    *val = ip + fi / fs;
    return 1;
}

static int c_parse(const char *s, double *out) {
    int i = 0, neg = 0;
    double v, den;
    if (s[0] == 0) return 0;
    if (s[i] == '-') { neg = 1; i++; }
    if (!c_num(s, &i, &v)) return 0;
    if (s[i] == '/') {
        i++;
        if (!c_num(s, &i, &den) || den == 0) return 0;
        v /= den;
    }
    if (s[i]) return 0;
    *out = neg ? -v : v;
    return 1;
}

/* ---------------------------------------------------------------- */
/*  Salida: cada linea es una formula en el lenguaje de gfx.h       */
/* ---------------------------------------------------------------- */
static void o_begin(Out *o) { o->n = 0; o->res = -1; o->err = 0; o->cl = 0; o->cur[0] = 0; }

static void o_s(Out *o, const char *s) {
    while (*s && o->cl < LINEW - 2) { o->cur[o->cl++] = *s++; }
    o->cur[o->cl] = 0;
}

/* numero en formato de formula (3.5E12 -> 3.5 x 10^12) */
static void o_n(Out *o, double x) {
    char b[40]; int i;
    c_fmt(x, b);
    for (i = 0; b[i]; i++) {
        if (b[i] == 'E') {
            o_s(o, "\\x10^{");
            o_s(o, b + i + 1);
            o_s(o, "}");
            return;
        }
        { char t[2]; t[0] = b[i]; t[1] = 0; o_s(o, t); }
    }
}

static void o_p(Out *o, double x) {            /* con parentesis si es negativo */
    if (x < 0) { o_s(o, "("); o_n(o, x); o_s(o, ")"); } else o_n(o, x);
}

static void o_end(Out *o) {
    if (o->n < MAXL) {
        int i;
        for (i = 0; i < o->cl; i++) o->l[o->n][i] = o->cur[i];
        o->l[o->n][o->cl] = 0;
        o->n++;
    }
    o->cl = 0; o->cur[0] = 0;
}

#define S(x)  o_s(o, x)
#define N(x)  o_p(o, x)
#define R(x)  o_n(o, x)
#define E()   o_end(o)

static int o_err(Out *o, const char *msg) {
    o_begin(o);
    o->err = 1;
    S("\\t{ERROR}"); E();
    S("\\t{"); S(msg); S("}"); E();
    return 0;
}

static void o_res(Out *o, const char *name, double x) {
    S(name); S("="); R(x); E();
    o->res = o->n - 1;
}

static void o_warn(Out *o, const char *msg) {
    S("\\t{"); S(msg); S("}"); E();
}

/* ---------------------------------------------------------------- */
/*  PROGRESION ARITMETICA                                           */
/* ---------------------------------------------------------------- */
/* v = {t1, n, r} */
static int f_pa_tn(const double *v, Out *o) {
    double t1 = v[0], n = v[1], r = v[2], a, b;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser un entero \xE2\x89\xA5 1");
    a = n - 1; b = a * r;
    o_begin(o);
    S("t_n=t_1+(n-1)r"); E();
    S("t_n="); N(t1); S("+("); R(n); S("-1)("); R(r); S(")"); E();
    S("t_n="); N(t1); S("+("); R(a); S(")("); R(r); S(")"); E();
    S("t_n="); N(t1); S("+"); N(b); E();
    o_res(o, "t_n", t1 + b);
    return 1;
}

/* v = {t1, tn, r} */
static int f_pa_n(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], r = v[2], d, q, n;
    if (r == 0) return o_err(o, "r no puede ser 0");
    d = tn - t1; q = d / r; n = q + 1;
    o_begin(o);
    S("n=\\f{t_n-t_1}{r}+1"); E();
    S("n=\\f{"); R(tn); S("-"); N(t1); S("}{"); N(r); S("}+1"); E();
    S("n=\\f{"); R(d); S("}{"); N(r); S("}+1"); E();
    S("n="); N(q); S("+1"); E();
    o_res(o, "n", n);
    if (!c_isint(n) || n < 1) o_warn(o, "Aviso: n no es entero positivo; los datos no forman una P.A.");
    return 1;
}

/* v = {t1, tn, n} */
static int f_pa_sn(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], n = v[2], s = t1 + tn, h = s / 2;
    o_begin(o);
    S("S_n=\\p{\\f{t_1+t_n}{2}}\\*n"); E();
    S("S_n=\\p{\\f{"); R(t1); S("+"); N(tn); S("}{2}}\\*"); N(n); E();
    S("S_n=\\p{\\f{"); R(s); S("}{2}}\\*"); N(n); E();
    S("S_n="); N(h); S("\\*"); N(n); E();
    o_res(o, "S_n", h * n);
    return 1;
}

/* v = {t1, n, r} */
static int f_pa_sn2(const double *v, Out *o) {
    double t1 = v[0], n = v[1], r = v[2], a, b, c, d;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser un entero \xE2\x89\xA5 1");
    a = 2 * t1; b = (n - 1) * r; c = a + b; d = n / 2;
    o_begin(o);
    S("S_n=\\f{n}{2}[2t_1+(n-1)r]"); E();
    S("S_n=\\f{"); R(n); S("}{2}[2("); R(t1); S(")+("); R(n); S("-1)("); R(r); S(")]"); E();
    S("S_n=\\f{"); R(n); S("}{2}["); N(a); S("+"); N(b); S("]"); E();
    S("S_n=\\f{"); R(n); S("}{2}("); R(c); S(")"); E();
    o_res(o, "S_n", d * c);
    return 1;
}

/* v = {t1, tn, n} */
static int f_pa_r(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], n = v[2], d, m;
    if (!c_isint(n) || n < 2) return o_err(o, "n debe ser un entero \xE2\x89\xA5 2");
    d = tn - t1; m = n - 1;
    o_begin(o);
    S("r=\\f{t_n-t_1}{n-1}"); E();
    S("r=\\f{"); R(tn); S("-"); N(t1); S("}{"); R(n); S("-1}"); E();
    S("r=\\f{"); R(d); S("}{"); R(m); S("}"); E();
    o_res(o, "r", d / m);
    return 1;
}

/* v = {tn, n, r} */
static int f_pa_t1(const double *v, Out *o) {
    double tn = v[0], n = v[1], r = v[2], m, b;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser un entero \xE2\x89\xA5 1");
    m = n - 1; b = m * r;
    o_begin(o);
    S("t_1=t_n-(n-1)r"); E();
    S("t_1="); R(tn); S("-("); R(n); S("-1)("); R(r); S(")"); E();
    S("t_1="); R(tn); S("-("); R(m); S(")("); R(r); S(")"); E();
    S("t_1="); R(tn); S("-"); N(b); E();
    o_res(o, "t_1", tn - b);
    return 1;
}

/* v = {t1, tn} */
static int f_pa_tm(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], s = t1 + tn;
    o_begin(o);
    S("t_m=\\f{t_1+t_n}{2}"); E();
    S("t_m=\\f{"); R(t1); S("+"); N(tn); S("}{2}"); E();
    S("t_m=\\f{"); R(s); S("}{2}"); E();
    o_res(o, "t_m", s / 2);
    return 1;
}

/* ---------------------------------------------------------------- */
/*  PROGRESION GEOMETRICA                                           */
/* ---------------------------------------------------------------- */
/* v = {t1, n, q} */
static int f_pg_tn(const double *v, Out *o) {
    double t1 = v[0], n = v[1], q = v[2], m, p;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser un entero \xE2\x89\xA5 1");
    m = n - 1; p = c_ipow(q, (long)m);
    o_begin(o);
    S("t_n=t_1\\*q^{n-1}"); E();
    S("t_n="); N(t1); S("\\*"); N(q); S("^{"); R(n); S("-1}"); E();
    S("t_n="); N(t1); S("\\*"); N(q); S("^{"); R(m); S("}"); E();
    S("t_n="); N(t1); S("\\*"); N(p); E();
    o_res(o, "t_n", t1 * p);
    return 1;
}

/* v = {t1, q, n} */
static int f_pg_sn(const double *v, Out *o) {
    double t1 = v[0], q = v[1], n = v[2], p, a, b;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser un entero \xE2\x89\xA5 1");
    o_begin(o);
    if (q == 1) {
        S("\\t{Como }q=1\\t{: }S_n=n\\*t_1"); E();
        S("S_n="); R(n); S("\\*"); N(t1); E();
        o_res(o, "S_n", n * t1);
        return 1;
    }
    p = c_ipow(q, (long)n); a = p - 1; b = q - 1;
    S("S_n=\\f{t_1(q^n-1)}{q-1}"); E();
    S("S_n=\\f{"); R(t1); S("("); N(q); S("^{"); R(n); S("}-1)}{"); N(q); S("-1}"); E();
    S("S_n=\\f{"); R(t1); S("("); R(p); S("-1)}{"); R(b); S("}"); E();
    S("S_n=\\f{"); R(t1 * a); S("}{"); R(b); S("}"); E();
    o_res(o, "S_n", t1 * a / b);
    return 1;
}

/* v = {t1, q} */
static int f_pg_s(const double *v, Out *o) {
    double t1 = v[0], q = v[1], d;
    if (q <= -1 || q >= 1) return o_err(o, "Se requiere  0 < q < 1 (|q| < 1)");
    d = 1 - q;
    o_begin(o);
    S("S=\\f{t_1}{1-q}"); E();
    S("S=\\f{"); R(t1); S("}{1-"); N(q); S("}"); E();
    S("S=\\f{"); R(t1); S("}{"); R(d); S("}"); E();
    o_res(o, "S", t1 / d);
    return 1;
}

/* v = {t1, tn, n} */
static int f_pg_pn(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], n = v[2], p, pw;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser un entero \xE2\x89\xA5 1");
    p = t1 * tn; pw = c_ipow(p, (long)n);
    if (pw < 0) return o_err(o, "(t1\xC2\xB7tn)^n es negativo: no hay ra\xC3\xADz real");
    o_begin(o);
    S("P_n=\\r{(t_1\\*t_n)^n}"); E();
    S("P_n=\\r{("); R(t1); S("\\*"); N(tn); S(")^{"); R(n); S("}}"); E();
    S("P_n=\\r{("); R(p); S(")^{"); R(n); S("}}"); E();
    S("P_n=\\r{"); R(pw); S("}"); E();
    o_res(o, "P_n", c_sqrt(pw));
    return 1;
}

/* v = {t1, tn, n} */
static int f_pg_q(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], n = v[2], ra, m, q;
    int neg = 0;
    if (!c_isint(n) || n < 2) return o_err(o, "n debe ser un entero \xE2\x89\xA5 2");
    if (t1 == 0) return o_err(o, "t1 no puede ser 0");
    ra = tn / t1; m = n - 1;
    if (ra == 0) return o_err(o, "tn no puede ser 0");
    if (ra < 0) {
        if (((long)m) % 2 == 0) return o_err(o, "No existe q real");
        neg = 1; ra = -ra;
    }
    q = c_exp(c_ln(ra) / m); if (neg) q = -q;
    o_begin(o);
    S("q=\\p{\\f{t_n}{t_1}}^{\\f{1}{n-1}}"); E();
    S("q=\\p{\\f{"); R(tn); S("}{"); R(t1); S("}}^{\\f{1}{"); R(n); S("-1}}"); E();
    S("q=\\p{"); R(tn / t1); S("}^{\\f{1}{"); R(m); S("}}"); E();
    o_res(o, "q", q);
    if (!neg && (((long)m) % 2 == 0)) { S("\\t{Tambi\xC3\xA9n existe }q=-"); R(q); E(); }
    return 1;
}

/* v = {t1, tn, q} */
static int f_pg_n(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], q = v[2], ra, a, b, n, rn, df;
    if (t1 == 0) return o_err(o, "t1 no puede ser 0");
    ra = tn / t1;
    if (ra <= 0)  return o_err(o, "tn/t1 debe ser mayor que 0");
    if (q <= 0 || q == 1) return o_err(o, "q debe ser > 0 y distinto de 1");
    a = c_ln(ra); b = c_ln(q); n = a / b + 1;
    o_begin(o);
    S("n=\\f{\\t{ln}\\p{\\f{t_n}{t_1}}}{\\t{ln}\\p{q}}+1"); E();
    S("n=\\f{\\t{ln}("); R(ra); S(")}{\\t{ln}("); R(q); S(")}+1"); E();
    S("n=\\f{"); R(a); S("}{"); R(b); S("}+1"); E();
    o_res(o, "n", n);
    rn = (double)((long)(n + 0.5));
    df = n - rn; if (df < 0) df = -df;
    if (df > 1e-6) o_warn(o, "Aviso: n no es entero; los datos no forman una P.G.");
    return 1;
}

/* ---------------------------------------------------------------- */
/*  Tabla de formulas                                               */
/* ---------------------------------------------------------------- */
typedef struct {
    int cat;                /* 0 = P.A., 1 = P.G.                            */
    const char *name;       /* nombre                                        */
    const char *form;       /* formula (lenguaje de gfx.h)                   */
    const char *use;        /* para que sirve                                */
    int nv;                 /* cantidad de variables                         */
    const char *vn[3];      /* simbolo de cada variable                      */
    const char *vd[3];      /* significado de cada simbolo                   */
    int (*fn)(const double *, Out *);
} Formula;

#define NFORM 13

static const char *CATS[2] = {
    "PROGRESI\xC3\x93N ARITM\xC3\x89TICA (P.A.)",
    "PROGRESI\xC3\x93N GEOM\xC3\x89TRICA (P.G.)"
};

static const Formula FORMS[NFORM] = {
 /* ------------------------- P.A. ------------------------- */
 {0, "T\xC3\xA9rmino en\xC3\xA9simo", "t_n=t_1+(n-1)r",
  "Calcula el valor del t\xC3\xA9rmino que ocupa la posici\xC3\xB3n n dentro de la progresi\xC3\xB3n.",
  3, {"t_1","n","r"}, {"Primer t\xC3\xA9rmino","Posici\xC3\xB3n del t\xC3\xA9rmino","Raz\xC3\xB3n aritm\xC3\xA9tica (+)"}, f_pa_tn},

 {0, "Cantidad de t\xC3\xA9rminos", "n=\\f{t_n-t_1}{r}+1",
  "Averigua cu\xC3\xA1ntos t\xC3\xA9rminos hay entre el primero (t1) y el t\xC3\xA9rmino tn.",
  3, {"t_1","t_n","r"}, {"Primer t\xC3\xA9rmino","T\xC3\xA9rmino en\xC3\xA9simo (\xC3\xBAltimo)","Raz\xC3\xB3n aritm\xC3\xA9tica (+)"}, f_pa_n},

 {0, "Suma de los n primeros t\xC3\xA9rminos", "S_n=\\p{\\f{t_1+t_n}{2}}\\*n",
  "Suma todos los t\xC3\xA9rminos, desde el primero hasta el que ocupa la posici\xC3\xB3n n.",
  3, {"t_1","t_n","n"}, {"Primer t\xC3\xA9rmino","T\xC3\xA9rmino en\xC3\xA9simo (\xC3\xBAltimo)","N\xC3\xBAmero de t\xC3\xA9rminos"}, f_pa_sn},

 {0, "Suma usando la raz\xC3\xB3n", "S_n=\\f{n}{2}[2t_1+(n-1)r]",
  "Suma los n primeros t\xC3\xA9rminos cuando no se conoce tn pero s\xC3\xAD la raz\xC3\xB3n r.",
  3, {"t_1","n","r"}, {"Primer t\xC3\xA9rmino","N\xC3\xBAmero de t\xC3\xA9rminos","Raz\xC3\xB3n aritm\xC3\xA9tica (+)"}, f_pa_sn2},

 {0, "Hallar la raz\xC3\xB3n (r)", "r=\\f{t_n-t_1}{n-1}",
  "Obtiene la raz\xC3\xB3n aritm\xC3\xA9tica conociendo el primer t\xC3\xA9rmino, el \xC3\xBAltimo y n.",
  3, {"t_1","t_n","n"}, {"Primer t\xC3\xA9rmino","T\xC3\xA9rmino en\xC3\xA9simo (\xC3\xBAltimo)","N\xC3\xBAmero de t\xC3\xA9rminos"}, f_pa_r},

 {0, "Hallar el primer t\xC3\xA9rmino", "t_1=t_n-(n-1)r",
  "Calcula t1 a partir de un t\xC3\xA9rmino tn, su posici\xC3\xB3n n y la raz\xC3\xB3n r.",
  3, {"t_n","n","r"}, {"T\xC3\xA9rmino en\xC3\xA9simo","Posici\xC3\xB3n del t\xC3\xA9rmino","Raz\xC3\xB3n aritm\xC3\xA9tica (+)"}, f_pa_t1},

 {0, "T\xC3\xA9rmino central", "t_m=\\f{t_1+t_n}{2}",
  "Calcula el t\xC3\xA9rmino del medio: es el promedio entre el primero y el \xC3\xBAltimo.",
  2, {"t_1","t_n",""}, {"Primer t\xC3\xA9rmino","\xC3\x9Altimo t\xC3\xA9rmino",""}, f_pa_tm},

 /* ------------------------- P.G. ------------------------- */
 {1, "T\xC3\xA9rmino en\xC3\xA9simo", "t_n=t_1\\*q^{n-1}",
  "Calcula el t\xC3\xA9rmino de la posici\xC3\xB3n n multiplicando t1 por la raz\xC3\xB3n q, (n\xE2\x88\x92" "1) veces.",
  3, {"t_1","n","q"}, {"Primer t\xC3\xA9rmino","Posici\xC3\xB3n del t\xC3\xA9rmino","Raz\xC3\xB3n geom\xC3\xA9trica (\xC3\x97)"}, f_pg_tn},

 {1, "Suma finita de n t\xC3\xA9rminos", "S_n=\\f{t_1(q^n-1)}{q-1}",
  "Suma los n primeros t\xC3\xA9rminos de la P.G. Si q = 1, la suma es simplemente n\xC2\xB7t1.",
  3, {"t_1","q","n"}, {"Primer t\xC3\xA9rmino","Raz\xC3\xB3n geom\xC3\xA9trica (\xC3\x97)","N\xC3\xBAmero de t\xC3\xA9rminos"}, f_pg_sn},

 {1, "Suma infinita decreciente", "S=\\f{t_1}{1-q}",
  "Suma de infinitos t\xC3\xA9rminos cuando 0 < q < 1: el resultado se acerca a un valor l\xC3\xADmite.",
  2, {"t_1","q",""}, {"Primer t\xC3\xA9rmino","Raz\xC3\xB3n geom\xC3\xA9trica (0 < q < 1)",""}, f_pg_s},

 {1, "Producto de n t\xC3\xA9rminos", "P_n=\\r{(t_1\\*t_n)^n}",
  "Multiplica los n primeros t\xC3\xA9rminos usando solo el primero y el \xC3\xBAltimo.",
  3, {"t_1","t_n","n"}, {"Primer t\xC3\xA9rmino","T\xC3\xA9rmino en\xC3\xA9simo (\xC3\xBAltimo)","N\xC3\xBAmero de t\xC3\xA9rminos"}, f_pg_pn},

 {1, "Hallar la raz\xC3\xB3n (q)", "q=\\p{\\f{t_n}{t_1}}^{\\f{1}{n-1}}",
  "Obtiene la raz\xC3\xB3n geom\xC3\xA9trica conociendo t1, tn y la cantidad de t\xC3\xA9rminos n.",
  3, {"t_1","t_n","n"}, {"Primer t\xC3\xA9rmino","T\xC3\xA9rmino en\xC3\xA9simo (\xC3\xBAltimo)","N\xC3\xBAmero de t\xC3\xA9rminos"}, f_pg_q},

 {1, "Hallar n (cantidad)", "n=\\f{\\t{ln}\\p{\\f{t_n}{t_1}}}{\\t{ln}\\p{q}}+1",
  "Averigua cu\xC3\xA1ntos t\xC3\xA9rminos tiene la P.G. a partir de t1, tn y la raz\xC3\xB3n q (usa logaritmos).",
  3, {"t_1","t_n","q"}, {"Primer t\xC3\xA9rmino","T\xC3\xA9rmino en\xC3\xA9simo (\xC3\xBAltimo)","Raz\xC3\xB3n geom\xC3\xA9trica (\xC3\x97)"}, f_pg_n},
};

#endif
