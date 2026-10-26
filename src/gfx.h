/* gfx.h - Dibujo directo en la VRAM (384x216, RGB565) y motor de formulas.
 * No usa funciones de Casio: solo necesita que se asigne g_vram.        */
#ifndef GFX_H
#define GFX_H

#include "font_data.h"

#define SCRW 384
#define SCRH 216
#define MKRGB(r, g, b) ((unsigned short)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

static unsigned short *g_vram;

/* ------------------------------------------------------------------ */
/*  Primitivas                                                         */
/* ------------------------------------------------------------------ */
static void gx_px(int x, int y, unsigned short c) {
    if ((unsigned)x < SCRW && (unsigned)y < SCRH) g_vram[y * SCRW + x] = c;
}

static void gx_rect(int x, int y, int w, int h, unsigned short c) {
    int i, j;
    for (j = y; j < y + h; j++) {
        if ((unsigned)j >= SCRH) continue;
        for (i = x; i < x + w; i++) if ((unsigned)i < SCRW) g_vram[j * SCRW + i] = c;
    }
}

static void gx_frame(int x, int y, int w, int h, unsigned short c) {
    gx_rect(x, y, w, 1, c); gx_rect(x, y + h - 1, w, 1, c);
    gx_rect(x, y, 1, h, c); gx_rect(x + w - 1, y, 1, h, c);
}

static void gx_line(int x0, int y0, int x1, int y1, unsigned short c) {
    int dx = x1 - x0, dy = y1 - y0, sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1, e, e2;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    e = dx - dy;
    for (;;) {
        gx_px(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * e;
        if (e2 > -dy) { e -= dy; x0 += sx; }
        if (e2 <  dx) { e += dx; y0 += sy; }
    }
}

/* ------------------------------------------------------------------ */
/*  Texto                                                              */
/* ------------------------------------------------------------------ */
static unsigned u8next(const char **p) {
    const unsigned char *s = (const unsigned char *)*p;
    unsigned c = s[0];
    if (c < 0x80)          { *p += 1; return c; }
    if ((c >> 5) == 6)     { *p += 2; return ((c & 31) << 6) | (s[1] & 63); }
    if ((c >> 4) == 14)    { *p += 3; return ((c & 15) << 12) | ((s[1] & 63) << 6) | (s[2] & 63); }
    *p += 1; return '?';
}

static const Glyph *g_find(const Font *f, unsigned cp) {
    int lo = 0, hi = f->n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        if (f->g[mid].cp == cp) return &f->g[mid];
        if (f->g[mid].cp < cp) lo = mid + 1; else hi = mid - 1;
    }
    return 0;
}

/* dibuja (o solo mide) un caracter; devuelve el avance en pixeles */
static int put_cp(const Font *f, unsigned cp, int x, int base, int draw,
                  unsigned short fg, unsigned short bg) {
    const Glyph *g = g_find(f, cp);
    if (!g) g = g_find(f, '?');
    if (!g) return 0;
    if (draw && g->w) {
        unsigned short pal[4];
        int r0 = (bg >> 11) & 31, g0 = (bg >> 5) & 63, b0 = bg & 31;
        int r1 = (fg >> 11) & 31, g1 = (fg >> 5) & 63, b1 = fg & 31, k, r, c;
        int rb = (g->w + 3) >> 2;
        for (k = 0; k < 4; k++)
            pal[k] = (unsigned short)(((r0 + (r1 - r0) * k / 3) << 11) |
                                      ((g0 + (g1 - g0) * k / 3) << 5) |
                                       (b0 + (b1 - b0) * k / 3));
        for (r = 0; r < f->h; r++)
            for (c = 0; c < g->w; c++) {
                int a = (f->d[g->off + r * rb + (c >> 2)] >> (6 - 2 * (c & 3))) & 3;
                if (a) gx_px(x + g->xoff + c, base - f->asc + r, pal[a]);
            }
    }
    return g->adv;
}

static int text_wn(const Font *f, const char *s, int n) {
    const char *e = s + n; int w = 0;
    while (s < e) w += put_cp(f, u8next(&s), 0, 0, 0, 0, 0);
    return w;
}
static int c_slen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static int text_w(const Font *f, const char *s) { return text_wn(f, s, c_slen(s)); }

static int text_draw(const Font *f, int x, int base, const char *s,
                     unsigned short fg, unsigned short bg) {
    int x0 = x;
    while (*s) x += put_cp(f, u8next(&s), x, base, 1, fg, bg);
    return x - x0;
}

/* texto con salto de linea automatico; devuelve la linea base siguiente */
static int text_wrap(const Font *f, int x, int base, int maxw, int lh, int indent,
                     const char *s, unsigned short fg, unsigned short bg) {
    int start = x + indent, cx = start, sp = text_w(f, " ");
    while (*s) {
        const char *e, *q; int w;
        while (*s == ' ') s++;
        e = s;
        while (*e && *e != ' ') e++;
        if (e == s) break;
        w = text_wn(f, s, (int)(e - s));
        if (cx != start && cx + w > x + maxw) { start = x; cx = x; base += lh; }
        q = s;
        while (q < e) cx += put_cp(f, u8next(&q), cx, base, 1, fg, bg);
        cx += sp;
        s = e;
    }
    return base + lh;
}

/* ------------------------------------------------------------------ */
/*  Motor de formulas                                                  */
/*   _x ^x  subindice / superindice   (x = caracter o {grupo})         */
/*   \f{a}{b}  fraccion     \r{a}  raiz cuadrada                       */
/*   \p{a}  parentesis que se estiran     \t{texto}  texto recto       */
/*   \*  punto medio    \x  signo por                                  */
/* ------------------------------------------------------------------ */
typedef struct { int w, asc, desc; } MBox;

static int M_DRAW;
static unsigned short M_FG = 0, M_BG = 0xFFFF;

static int m_isdesc(unsigned cp) {
    const char *d = "gjpqyQ,;()[]";
    int i;
    for (i = 0; d[i]; i++) if ((unsigned)d[i] == cp) return 1;
    return 0;
}

static void m_seq(const char **p, int level, int fd, int tm, int x, int base, char stop, MBox *b);

static const char *m_group(const char *q, int level, int fd, int tm, int x, int base, MBox *b) {
    m_seq(&q, level, fd, tm, x, base, '}', b);
    return q;
}
static const char *m_meas(const char *q, int level, int fd, int tm, MBox *b) {
    int o = M_DRAW; const char *e;
    M_DRAW = 0; e = m_group(q, level, fd, tm, 0, 0, b); M_DRAW = o;
    return e;
}

static void m_glyph(unsigned cp, int level, int tm, int x, int base, MBox *b) {
    const Font *f;
    int italic = (!tm && ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z')));
    if (!tm && cp == '-' && !level) cp = 0x2212;   /* la fuente pequena no trae el signo menos largo */
    f = level ? (italic ? &F_SI : &F_S) : (italic ? &F_I : &F_N);
    b->w = put_cp(f, cp, x, base, M_DRAW, M_FG, M_BG);
    b->asc = f->cap + 1;
    b->desc = m_isdesc(cp) ? f->dsc : 0;
}

static void m_atom(const char **p, int level, int fd, int tm, int x, int base, MBox *b) {
    char c = **p;
    b->w = 0; b->asc = 0; b->desc = 0;
    if (c == '{') {
        *p = m_group(*p + 1, level, fd, tm, x, base, b);
        return;
    }
    if (c == '\\') {
        char k = (*p)[1];
        *p += 2;
        if (k == '*') { m_glyph(0xB7, level, 1, x, base, b); return; }
        if (k == 'x') { m_glyph(0xD7, level, 1, x, base, b); return; }
        if (k == 't') { if (**p == '{') *p = m_group(*p + 1, level, fd, 1, x, base, b); return; }
        if (k == 'f') {
            MBox A, B; const char *qa, *qb, *e1, *e2;
            int l2 = (level || fd >= 1) ? 1 : 0, axis = level ? 3 : 4;
            int w, bar, nb, db;
            qa = *p + 1;
            e1 = m_meas(qa, l2, fd + 1, tm, &A);
            qb = e1 + 1;
            e2 = m_meas(qb, l2, fd + 1, tm, &B);
            w = (A.w > B.w ? A.w : B.w) + 4;
            bar = base - axis;
            nb = bar - 2 - A.desc;
            db = bar + 2 + B.asc + 1;
            b->w = w;
            b->asc = base - (nb - A.asc);
            b->desc = db + B.desc - base;
            if (M_DRAW) {
                MBox t;
                m_group(qa, l2, fd + 1, tm, x + (w - A.w) / 2, nb, &t);
                m_group(qb, l2, fd + 1, tm, x + (w - B.w) / 2, db, &t);
                gx_rect(x + 1, bar, w - 2, 1, M_FG);
            }
            *p = e2;
            return;
        }
        if (k == 'r') {
            MBox I; const char *e; int top, bot, mid, ox = 9;
            if (**p != '{') return;
            e = m_meas(*p + 1, level, fd, tm, &I);
            top = base - I.asc - 3; bot = base + I.desc + 1; mid = top + (bot - top) * 55 / 100;
            if (M_DRAW) {
                MBox t;
                m_group(*p + 1, level, fd, tm, x + ox, base, &t);
                gx_line(x, mid, x + 3, bot, M_FG);
                gx_line(x + 3, bot, x + 7, top, M_FG);
                gx_line(x + 4, bot, x + 8, top, M_FG);
                gx_rect(x + 7, top, I.w + ox - 7 + 2, 1, M_FG);
            }
            b->w = ox + I.w + 3; b->asc = I.asc + 4; b->desc = I.desc + 1;
            *p = e;
            return;
        }
        if (k == 'p') {
            MBox I; const char *e; int h;
            if (**p != '{') return;
            e = m_meas(*p + 1, level, fd, tm, &I);
            h = I.asc + I.desc;
            if (h <= 15) {
                MBox a1, a2, t; int xi;
                m_glyph('(', level, 1, x, base, &a1);
                xi = x + a1.w;
                if (M_DRAW) m_group(*p + 1, level, fd, tm, xi, base, &t);
                m_glyph(')', level, 1, xi + I.w, base, &a2);
                b->w = a1.w + I.w + a2.w;
                b->asc = I.asc > a1.asc ? I.asc : a1.asc;
                b->desc = I.desc > a1.desc ? I.desc : a1.desc;
            } else {
                int top = base - I.asc - 1, bot = base + I.desc + 1, q = (bot - top) / 4, mid = (top + bot) / 2;
                int ix = x + 6, rx = ix + I.w + 2;
                if (M_DRAW) {
                    MBox t;
                    m_group(*p + 1, level, fd, tm, ix, base, &t);
                    gx_line(x + 4, top, x + 1, top + q, M_FG); gx_line(x + 1, top + q, x + 1, bot - q, M_FG);
                    gx_line(x + 1, bot - q, x + 4, bot, M_FG);
                    gx_line(rx + 1, top, rx + 4, top + q, M_FG); gx_line(rx + 4, top + q, rx + 4, bot - q, M_FG);
                    gx_line(rx + 4, bot - q, rx + 1, bot, M_FG);
                    (void)mid;
                }
                b->w = I.w + 12; b->asc = I.asc + 2; b->desc = I.desc + 2;
            }
            *p = e;
            return;
        }
        return;
    }
    {
        unsigned cp = u8next(p);
        m_glyph(cp, level, tm, x, base, b);
    }
}

static void m_seq(const char **p, int level, int fd, int tm, int x, int base, char stop, MBox *b) {
    int x0 = x, lastc = 0, asc = 0, desc = 0;
    for (;;) {
        char c = **p;
        MBox a; int gap = level ? 2 : 3, pre = 0, post = 0, sw, up, sh;
        if (c == 0) break;
        if (c == stop && stop) { (*p)++; break; }
        if (c == '}') { (*p)++; continue; }
        if (!tm) {
            if (c == '=' || c == '+') { pre = post = gap; }
            else if (c == '-') {
                if (lastc && lastc != '(' && lastc != '[' && lastc != '=' && lastc != '+' && lastc != '-' && lastc != '*')
                    pre = post = gap;
            }
            else if (c == '\\' && (*p)[1] == '*') { pre = post = 1; }
        }
        x += pre;
        m_atom(p, level, fd, tm, x, base, &a);
        x += a.w;
        if (a.asc > asc) asc = a.asc;
        if (a.desc > desc) desc = a.desc;
        x += post;
        lastc = (c == '\\') ? ((*p)[-1] == '*' ? '*' : 'a') : c;
        sw = 0;
        while (**p == '_' || **p == '^') {
            MBox s;
            up = (**p == '^'); (*p)++;
            sh = up ? -(level ? 3 : 5) : (level ? 2 : 3);
            m_atom(p, 1, fd, tm, x, base + sh, &s);
            if (s.w > sw) sw = s.w;
            if (up) { if (s.asc - sh > asc) asc = s.asc - sh; }
            else    { if (s.desc + sh > desc) desc = s.desc + sh; }
        }
        x += sw;
        if (sw) x += 1;
    }
    b->w = x - x0; b->asc = asc; b->desc = desc;
}

static void math_measure(const char *s, MBox *b) {
    int o = M_DRAW; M_DRAW = 0;
    m_seq(&s, 0, 0, 0, 0, 0, 0, b);
    M_DRAW = o;
}

static void math_draw(const char *s, int x, int base, unsigned short fg, unsigned short bg) {
    MBox b; int of = M_FG, ob = M_BG, od = M_DRAW;
    M_FG = fg; M_BG = bg; M_DRAW = 1;
    m_seq(&s, 0, 0, 0, x, base, 0, &b);
    M_FG = (unsigned short)of; M_BG = (unsigned short)ob; M_DRAW = od;
}

/* dibuja la formula centrada dentro de un rectangulo */
static void math_center(const char *s, int rx, int ry, int rw, int rh,
                        unsigned short fg, unsigned short bg) {
    MBox b;
    math_measure(s, &b);
    math_draw(s, rx + (rw - b.w) / 2, ry + (rh - (b.asc + b.desc)) / 2 + b.asc, fg, bg);
}

#endif
