/* core.h - Logica de calculo de progresiones (sin dependencias de Casio).
 * Se puede probar en PC con tests/test_core.c                          */
#ifndef CORE_H
#define CORE_H

typedef unsigned long long u64;

#define COLS    21          /* columnas de texto por fila en la fx-CG50 */
#define MAXROWS 60          /* filas maximas del procedimiento          */

typedef struct {
    char rows[MAXROWS][COLS + 1];
    int  n;
    char cur[96];
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
/*  Salida con ajuste de linea                                      */
/* ---------------------------------------------------------------- */
static void o_begin(Out *o) { o->n = 0; o->cl = 0; o->cur[0] = 0; }

static void o_s(Out *o, const char *s) {
    while (*s && o->cl < 94) { o->cur[o->cl++] = *s++; }
    o->cur[o->cl] = 0;
}

static void o_n(Out *o, double x) { char b[40]; c_fmt(x, b); o_s(o, b); }

static void o_p(Out *o, double x) {            /* con parentesis si es negativo */
    if (x < 0) { o_s(o, "("); o_n(o, x); o_s(o, ")"); } else o_n(o, x);
}

static void o_push(Out *o, const char *s, int len) {
    int i;
    if (o->n >= MAXROWS) return;
    for (i = 0; i < len && i < COLS; i++) o->rows[o->n][i] = s[i];
    o->rows[o->n][i] = 0;
    o->n++;
}

static void o_end(Out *o) {
    int pos = 0, len = o->cl, take, sp, i;
    if (len == 0) o_push(o, "", 0);
    while (pos < len) {
        if (len - pos <= COLS) { take = len - pos; o_push(o, o->cur + pos, take); pos = len; }
        else {
            sp = -1;
            for (i = pos + COLS; i > pos; i--) if (o->cur[i] == ' ') { sp = i; break; }
            if (sp > pos) { o_push(o, o->cur + pos, sp - pos); pos = sp + 1; }
            else          { o_push(o, o->cur + pos, COLS);     pos += COLS; }
        }
    }
    o->cl = 0; o->cur[0] = 0;
}

#define S(x)  o_s(o, x)
#define N(x)  o_p(o, x)
#define R(x)  o_n(o, x)
#define E()   o_end(o)

static int o_err(Out *o, const char *msg) {
    o_begin(o);
    S("ERROR:"); E();
    S(msg); E();
    return 0;
}

static void o_res(Out *o, const char *name, double x) {
    S("=> "); S(name); S(" = "); R(x); E();
}

/* ---------------------------------------------------------------- */
/*  PROGRESION ARITMETICA                                           */
/* ---------------------------------------------------------------- */
/* v = {t1, n, r} */
static int f_pa_tn(const double *v, Out *o) {
    double t1 = v[0], n = v[1], r = v[2], a, b;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser entero >= 1");
    a = n - 1; b = a * r;
    o_begin(o);
    S("tn = t1 + (n-1)r"); E();
    S("tn = "); N(t1); S(" + ("); R(n); S("-1)("); R(r); S(")"); E();
    S("tn = "); N(t1); S(" + ("); R(a); S(")("); R(r); S(")"); E();
    S("tn = "); N(t1); S(" + "); N(b); E();
    o_res(o, "tn", t1 + b);
    return 1;
}

/* v = {t1, tn, r} */
static int f_pa_n(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], r = v[2], d, q, n;
    if (r == 0) return o_err(o, "r no puede ser 0");
    d = tn - t1; q = d / r; n = q + 1;
    o_begin(o);
    S("n = (tn-t1)/r + 1"); E();
    S("n = ("); R(tn); S("-"); N(t1); S(")/"); N(r); S(" + 1"); E();
    S("n = "); N(d); S("/"); N(r); S(" + 1"); E();
    S("n = "); N(q); S(" + 1"); E();
    o_res(o, "n", n);
    if (!c_isint(n) || n < 1) { S("Aviso: n no es entero"); E(); S("positivo, datos no"); E(); S("forman una P.A."); E(); }
    return 1;
}

/* v = {t1, tn, n} */
static int f_pa_sn(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], n = v[2], s = t1 + tn, h = s / 2;
    o_begin(o);
    S("Sn = ((t1+tn)/2)n"); E();
    S("Sn = (("); R(t1); S("+"); N(tn); S(")/2)("); R(n); S(")"); E();
    S("Sn = ("); N(s); S("/2)("); R(n); S(")"); E();
    S("Sn = ("); N(h); S(")("); R(n); S(")"); E();
    o_res(o, "Sn", h * n);
    return 1;
}

/* v = {t1, n, r} */
static int f_pa_sn2(const double *v, Out *o) {
    double t1 = v[0], n = v[1], r = v[2], a, b, c, d;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser entero >= 1");
    a = 2 * t1; b = (n - 1) * r; c = a + b; d = n / 2;
    o_begin(o);
    S("Sn = n/2[2t1+(n-1)r]"); E();
    S("Sn = "); R(n); S("/2[2("); R(t1); S(")+("); R(n); S("-1)("); R(r); S(")]"); E();
    S("Sn = "); N(d); S("[ "); N(a); S(" + "); N(b); S(" ]"); E();
    S("Sn = "); N(d); S("("); R(c); S(")"); E();
    o_res(o, "Sn", d * c);
    return 1;
}

/* v = {t1, tn, n} */
static int f_pa_r(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], n = v[2], d, m;
    if (!c_isint(n) || n < 2) return o_err(o, "n debe ser entero >= 2");
    d = tn - t1; m = n - 1;
    o_begin(o);
    S("r = (tn-t1)/(n-1)"); E();
    S("r = ("); R(tn); S("-"); N(t1); S(")/("); R(n); S("-1)"); E();
    S("r = "); N(d); S("/"); N(m); E();
    o_res(o, "r", d / m);
    return 1;
}

/* v = {tn, n, r} */
static int f_pa_t1(const double *v, Out *o) {
    double tn = v[0], n = v[1], r = v[2], m, b;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser entero >= 1");
    m = n - 1; b = m * r;
    o_begin(o);
    S("t1 = tn - (n-1)r"); E();
    S("t1 = "); R(tn); S(" - ("); R(n); S("-1)("); R(r); S(")"); E();
    S("t1 = "); R(tn); S(" - ("); R(m); S(")("); R(r); S(")"); E();
    S("t1 = "); R(tn); S(" - "); N(b); E();
    o_res(o, "t1", tn - b);
    return 1;
}

/* v = {t1, tn} */
static int f_pa_tm(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], s = t1 + tn;
    o_begin(o);
    S("tm = (t1 + tn)/2"); E();
    S("tm = ("); R(t1); S(" + "); N(tn); S(")/2"); E();
    S("tm = "); N(s); S("/2"); E();
    o_res(o, "tm", s / 2);
    return 1;
}

/* ---------------------------------------------------------------- */
/*  PROGRESION GEOMETRICA                                           */
/* ---------------------------------------------------------------- */
/* v = {t1, n, q} */
static int f_pg_tn(const double *v, Out *o) {
    double t1 = v[0], n = v[1], q = v[2], m, p;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser entero >= 1");
    m = n - 1; p = c_ipow(q, (long)m);
    o_begin(o);
    S("tn = t1 . q^(n-1)"); E();
    S("tn = "); R(t1); S(" . ("); R(q); S(")^("); R(n); S("-1)"); E();
    S("tn = "); R(t1); S(" . ("); R(q); S(")^"); R(m); E();
    S("tn = "); R(t1); S(" . "); N(p); E();
    o_res(o, "tn", t1 * p);
    return 1;
}

/* v = {t1, q, n} */
static int f_pg_sn(const double *v, Out *o) {
    double t1 = v[0], q = v[1], n = v[2], p, a, b;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser entero >= 1");
    o_begin(o);
    if (q == 1) {
        S("q = 1 -> Sn = n.t1"); E();
        S("Sn = "); R(n); S(" . "); N(t1); E();
        o_res(o, "Sn", n * t1);
        return 1;
    }
    p = c_ipow(q, (long)n); a = p - 1; b = q - 1;
    S("Sn = t1(q^n - 1)"); E();
    S("      ----------"); E();
    S("         q - 1"); E();
    S("Sn = "); R(t1); S("(("); R(q); S(")^"); R(n); S("-1)/("); R(q); S("-1)"); E();
    S("Sn = "); R(t1); S("("); N(p); S("-1)/("); N(b); S(")"); E();
    S("Sn = "); R(t1); S("("); N(a); S(")/("); N(b); S(")"); E();
    S("Sn = "); N(t1 * a); S("/"); N(b); E();
    o_res(o, "Sn", t1 * a / b);
    return 1;
}

/* v = {t1, q} */
static int f_pg_s(const double *v, Out *o) {
    double t1 = v[0], q = v[1], d;
    if (q <= -1 || q >= 1) return o_err(o, "Se requiere |q| < 1");
    d = 1 - q;
    o_begin(o);
    S("S = t1 / (1 - q)"); E();
    S("S = "); R(t1); S(" / (1 - "); N(q); S(")"); E();
    S("S = "); R(t1); S(" / "); N(d); E();
    o_res(o, "S", t1 / d);
    return 1;
}

/* v = {t1, tn, n} */
static int f_pg_pn(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], n = v[2], p, pw;
    if (!c_isint(n) || n < 1) return o_err(o, "n debe ser entero >= 1");
    p = t1 * tn; pw = c_ipow(p, (long)n);
    if (pw < 0) return o_err(o, "(t1.tn)^n es negativo");
    o_begin(o);
    S("Pn = sqrt[(t1.tn)^n]"); E();
    S("Pn = sqrt[("); R(t1); S(" . "); N(tn); S(")^"); R(n); S("]"); E();
    S("Pn = sqrt[("); N(p); S(")^"); R(n); S("]"); E();
    S("Pn = sqrt["); N(pw); S("]"); E();
    o_res(o, "Pn", c_sqrt(pw));
    return 1;
}

/* v = {t1, tn, n} */
static int f_pg_q(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], n = v[2], ra, m, q;
    int neg = 0;
    if (!c_isint(n) || n < 2) return o_err(o, "n debe ser entero >= 2");
    if (t1 == 0) return o_err(o, "t1 no puede ser 0");
    ra = tn / t1; m = n - 1;
    if (ra == 0) return o_err(o, "tn no puede ser 0");
    if (ra < 0) {
        if (((long)m) % 2 == 0) return o_err(o, "No existe q real");
        neg = 1; ra = -ra;
    }
    q = c_exp(c_ln(ra) / m); if (neg) q = -q;
    o_begin(o);
    S("q = (tn/t1)^(1/(n-1))"); E();
    S("q = ("); R(tn); S("/"); N(t1); S(")^(1/"); R(m); S(")"); E();
    S("q = ("); R(tn / t1); S(")^(1/"); R(m); S(")"); E();
    o_res(o, "q", q);
    if (!neg && (((long)m) % 2 == 0)) { S("(tambien q = -"); R(q); S(")"); E(); }
    return 1;
}

/* v = {t1, tn, q} */
static int f_pg_n(const double *v, Out *o) {
    double t1 = v[0], tn = v[1], q = v[2], ra, a, b, n;
    if (t1 == 0) return o_err(o, "t1 no puede ser 0");
    ra = tn / t1;
    if (ra <= 0)  return o_err(o, "tn/t1 debe ser > 0");
    if (q <= 0 || q == 1) return o_err(o, "q debe ser > 0 y q<>1");
    a = c_ln(ra); b = c_ln(q); n = a / b + 1;
    o_begin(o);
    S("n = log(tn/t1) / log(q) + 1"); E();
    S("n = log("); R(ra); S(") / log("); R(q); S(") + 1"); E();
    S("n = "); R(a); S(" / "); R(b); S(" + 1"); E();
    S("(log natural)"); E();
    o_res(o, "n", n);
    {
        double rn = (double)((long)(n + 0.5));
        double df = n - rn; if (df < 0) df = -df;
        if (df > 1e-6) { S("Aviso: n no es entero"); E(); }
    }
    return 1;
}

/* ---------------------------------------------------------------- */
/*  Tabla de formulas                                               */
/* ---------------------------------------------------------------- */
typedef struct {
    const char *cat;        /* categoria                         */
    const char *desc;       /* descripcion (max 21 car.)         */
    const char *form;       /* formula corta para el menu        */
    int nv;                 /* cantidad de variables             */
    const char *vn[3];      /* nombre de cada variable           */
    const char *vd[3];      /* descripcion de cada variable      */
    int (*fn)(const double *, Out *);
} Formula;

#define NFORM 13

static const Formula FORMS[NFORM] = {
 /* ---- P.A. ---- */
 {"PROG. ARITMETICA", "Termino enesimo",       "tn=t1+(n-1)r",   3, {"t1","n","r"},   {"Primer termino","Posicion n","Razon aritmetica r"}, f_pa_tn},
 {"PROG. ARITMETICA", "Cantidad de terminos",  "n=(tn-t1)/r+1",  3, {"t1","tn","r"},  {"Primer termino","Termino enesimo","Razon aritmetica r"}, f_pa_n},
 {"PROG. ARITMETICA", "Suma de n terminos",    "Sn=((t1+tn)/2)n",3, {"t1","tn","n"},  {"Primer termino","Termino enesimo","Numero de terminos"}, f_pa_sn},
 {"PROG. ARITMETICA", "Suma usando la razon",  "Sn=n/2[2t1+(n-1)r]",3,{"t1","n","r"}, {"Primer termino","Numero de terminos","Razon aritmetica r"}, f_pa_sn2},
 {"PROG. ARITMETICA", "Hallar la razon r",     "r=(tn-t1)/(n-1)",3, {"t1","tn","n"},  {"Primer termino","Termino enesimo","Numero de terminos"}, f_pa_r},
 {"PROG. ARITMETICA", "Hallar el 1er termino", "t1=tn-(n-1)r",   3, {"tn","n","r"},   {"Termino enesimo","Numero de terminos","Razon aritmetica r"}, f_pa_t1},
 {"PROG. ARITMETICA", "Termino central",       "tm=(t1+tn)/2",   2, {"t1","tn",""},   {"Primer termino","Ultimo termino",""}, f_pa_tm},
 /* ---- P.G. ---- */
 {"PROG. GEOMETRICA", "Termino enesimo",       "tn=t1.q^(n-1)",  3, {"t1","n","q"},   {"Primer termino","Posicion n","Razon geometrica q"}, f_pg_tn},
 {"PROG. GEOMETRICA", "Suma finita de n term.","Sn=t1(q^n-1)/(q-1)",3,{"t1","q","n"}, {"Primer termino","Razon geometrica q","Numero de terminos"}, f_pg_sn},
 {"PROG. GEOMETRICA", "Suma infinita decrec.", "S=t1/(1-q)",     2, {"t1","q",""},    {"Primer termino","Razon q (|q|<1)",""}, f_pg_s},
 {"PROG. GEOMETRICA", "Producto de n terminos","Pn=sqrt[(t1.tn)^n]",3,{"t1","tn","n"}, {"Primer termino","Termino enesimo","Numero de terminos"}, f_pg_pn},
 {"PROG. GEOMETRICA", "Hallar la razon q",     "q=(tn/t1)^(1/(n-1))",3,{"t1","tn","n"},{"Primer termino","Termino enesimo","Numero de terminos"}, f_pg_q},
 {"PROG. GEOMETRICA", "Hallar n (cantidad)",   "n=log(tn/t1)/log q+1",3,{"t1","tn","q"},{"Primer termino","Termino enesimo","Razon geometrica q"}, f_pg_n},
};

#endif
