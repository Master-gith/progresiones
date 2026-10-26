/* ui.h - Pantallas (menu, formula/entrada, procedimiento). Sin llamadas a Casio. */
#ifndef UI_H
#define UI_H

#include "gfx.h"
#include "core.h"

#define C_WHITE  RGB(255,255,255)
#define C_BLACK  RGB(0,0,0)
#define C_DARK   RGB(34,40,46)
#define C_BAR    RGB(60,66,74)
#define C_PA     RGB(255,196,92)
#define C_PG     RGB(138,184,238)
#define C_PA_L   RGB(255,236,200)
#define C_PG_L   RGB(214,232,252)
#define C_GRAY   RGB(95,95,95)
#define C_LGRAY  RGB(185,190,196)
#define C_RED    RGB(200,30,30)
#define C_REDBAR RGB(190,40,40)
#define C_GREENL RGB(212,244,214)
#define C_GREEN  RGB(40,140,60)
#define C_BLUE   RGB(25,70,160)
#define C_YEL    RGB(255,246,196)
#define C_ORANGE RGB(235,140,20)

#define VLEN 15

static unsigned short cat_col(int cat)  { return cat ? C_PG : C_PA; }
static unsigned short cat_light(int cat){ return cat ? C_PG_L : C_PA_L; }

static void ui_header(const char *t, int cat) {
    unsigned short c = cat_col(cat);
    int w = text_w(&F_B, t);
    rect(0, 0, SW, 24, c);
    rect(0, 24, SW, 1, C_DARK);
    text_draw(&F_B, (SW - w) / 2, 17, t, C_BLACK, c);
}

static void ui_bar(const char *t, unsigned short bg) {
    int w = text_w(&F_N, t);
    rect(0, 197, SW, 19, bg);
    text_draw(&F_N, (SW - w) / 2, 211, t, C_WHITE, bg);
}

/* ---------------- menu principal ---------------- */
static void draw_menu(int sel, int top) {
    int i, y;
    rect(0, 0, SW, SH, C_WHITE);
    ui_header(CATS[FORMS[sel].cat], FORMS[sel].cat);
    for (i = 0; i < 6; i++) {
        int idx = top + i, cat;
        char b[64]; int p = 0, j = 0;
        y = 28 + i * 17;
        if (idx >= NFORM) break;
        cat = FORMS[idx].cat;
        if (idx == sel) {
            rect(2, y, 370, 16, cat_light(cat));
            frame(2, y, 370, 16, cat_col(cat));
        }
        rect(7, y + 4, 8, 8, cat_col(cat));
        if (idx + 1 >= 10) b[p++] = '0' + (idx + 1) / 10;
        b[p++] = '0' + (idx + 1) % 10;
        b[p++] = '.'; b[p++] = ' ';
        while (FORMS[idx].name[j] && p < 60) b[p++] = FORMS[idx].name[j++];
        b[p] = 0;
        text_draw(idx == sel ? &F_B : &F_N, 22, y + 13, b, C_BLACK, idx == sel ? cat_light(cat) : C_WHITE);
    }
    /* barra de desplazamiento */
    rect(377, 28, 4, 102, C_PA_L);
    rect(377, 28 + top * 102 / NFORM, 4, 6 * 102 / NFORM, C_ORANGE);
    /* vista previa de la formula */
    rect(0, 134, SW, 82, C_DARK);
    rect(0, 134, SW, 2, cat_col(FORMS[sel].cat));
    math_center(FORMS[sel].form, 4, 138, 376, 62, C_WHITE, C_DARK);
    {
        const char *h = "EXE: abrir   EXIT: salir";
        text_draw(&F_S, SW - 8 - text_w(&F_S, h), 212, h, RGB(170,180,190), C_DARK);
    }
}

/* ---------------- formula + entrada de datos ---------------- */
static void draw_input(int fi, int sel, const char *msg, char vals[3][VLEN + 1]) {
    const Formula *f = &FORMS[fi];
    int i, ind;
    rect(0, 0, SW, SH, C_WHITE);
    ui_header(f->name, f->cat);
    /* panel de la formula (como la pizarra de la hoja) */
    rect(0, 25, SW, 55, C_DARK);
    frame(0, 25, SW, 55, cat_col(f->cat));
    math_center(f->form, 4, 27, 376, 51, C_WHITE, C_DARK);
    /* para que sirve */
    ind = text_w(&F_B, "Sirve para:") + 4;
    text_draw(&F_B, 8, 96, "Sirve para:", C_BLUE, C_WHITE);
    text_wrap(&F_N, 8, 96, 368, 15, ind, f->use, C_BLACK, C_WHITE);
    /* simbologia: simbolo = valor   significado */
    for (i = 0; i < f->nv; i++) {
        int y = 132 + i * 19, on = (i == sel), bw = 100, tx;
        unsigned short bg = on ? C_YEL : C_WHITE;
        math_draw(f->vn[i], 12, y + 13, C_BLACK, C_WHITE);
        text_draw(&F_N, 38, y + 13, "=", C_BLACK, C_WHITE);
        rect(50, y, bw, 17, bg);
        frame(50, y, bw, 17, on ? C_ORANGE : C_LGRAY);
        tx = 55 + text_draw(&F_N, 55, y + 13, vals[i], C_BLACK, bg);
        if (on) rect(tx, y + 3, 1, 12, C_BLACK);
        text_draw(on ? &F_B : &F_N, 160, y + 13, f->vd[i], on ? C_BLACK : C_GRAY, C_WHITE);
    }
    if (msg[0]) ui_bar(msg, C_REDBAR);
    else ui_bar("EXE Calcular   \xE2\x96\xB2\xE2\x96\xBC Cambiar   DEL Borrar   EXIT Men\xC3\xBA", C_BAR);
}

/* ---------------- procedimiento ---------------- */
#define RES_TOP 28
#define RES_BOT 194

static int res_layout(const Out *o, int top, int draw) {
    int i, y = RES_TOP, shown = 0;
    for (i = top; i < o->n; i++) {
        Box b; int lh, isres = (i == o->res);
        math_measure(o->l[i], &b);
        lh = b.asc + b.desc + 10;
        if (y + lh > RES_BOT && shown > 0) break;
        if (draw) {
            unsigned short bg = isres ? C_GREENL : C_WHITE;
            unsigned short fg = o->err ? C_RED : (i == 0 ? C_BLUE : C_BLACK);
            if (isres) { rect(4, y, 366, lh - 2, C_GREENL); frame(4, y, 366, lh - 2, C_GREEN); }
            math_draw(o->l[i], 14, y + 4 + b.asc, fg, bg);
        }
        y += lh; shown++;
    }
    return shown;
}

static void draw_result(const Out *o, int top, int fi) {
    int shown;
    rect(0, 0, SW, SH, C_WHITE);
    ui_header(FORMS[fi].name, FORMS[fi].cat);
    shown = res_layout(o, top, 1);
    if (top > 0) text_draw(&F_N, 368, 44, "\xE2\x96\xB2", C_GRAY, C_WHITE);
    if (top + shown < o->n) text_draw(&F_N, 368, 190, "\xE2\x96\xBC", C_GRAY, C_WHITE);
    ui_bar("\xE2\x96\xB2\xE2\x96\xBC Desplazar   EXE / EXIT Volver", C_BAR);
}

#endif
