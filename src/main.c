/* Progresiones P.A. / P.G. para Casio fx-CG50 (add-in .g3a)
 * Usa libfxcg (PrizmSDK). Pantalla de texto: 21 columnas x 8 filas.   */
#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include "core.h"

#define VLEN 15

static char  vbuf[NFORM][3][VLEN + 1];   /* los valores se conservan   */
static Out   out;

/* ---------- dibujo ---------- */
static void pr(int row, const char *t, int invert) {
    char b[32]; int i = 0, j = 0;
    b[i++] = '-'; b[i++] = '-';
    while (t[j] && i < 2 + COLS) b[i++] = t[j++];
    if (invert) while (i < 2 + COLS) b[i++] = ' ';
    b[i] = 0;
    PrintXY(1, row, b, invert ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
}

static int getk(void) { int k; GetKey(&k); return k; }

/* ---------- pantalla de resultado ---------- */
static void screen_result(void) {
    int top = 0, k, i, vis = 7;
    for (;;) {
        Bdisp_AllClr_VRAM();
        for (i = 0; i < vis && top + i < out.n; i++) pr(1 + i, out.rows[top + i], 0);
        pr(8, out.n > vis ? "UP/DN:mover EXIT:atras" : "EXIT/EXE: volver", 1);
        Bdisp_PutDisp_DD();
        k = getk();
        if (k == KEY_CTRL_EXIT || k == KEY_CTRL_EXE) return;
        if (k == KEY_CTRL_DOWN && top + vis < out.n) top++;
        if (k == KEY_CTRL_UP && top > 0) top--;
    }
}

/* ---------- entrada de datos ---------- */
static char key_to_char(int k) {
    if (k >= KEY_CHAR_0 && k <= KEY_CHAR_9) return (char)('0' + (k - KEY_CHAR_0));
    if (k == KEY_CHAR_DP) return '.';
    if (k == KEY_CHAR_PMINUS || k == KEY_CHAR_MINUS) return '-';
    if (k == KEY_CHAR_DIV) return '/';
    return 0;
}

static void edit_append(char *s, char c) {
    int n = c_len(s), i, slash = -1;
    if (n >= VLEN) return;
    for (i = 0; i < n; i++) if (s[i] == '/') slash = i;
    if (c == '-') { if (n != 0) return; }
    else if (c == '/') {
        if (slash >= 0 || n == 0 || s[n - 1] < '0' || s[n - 1] > '9') return;
    } else if (c == '.') {
        for (i = (slash >= 0 ? slash + 1 : 0); i < n; i++) if (s[i] == '.') return;
    }
    s[n] = c; s[n + 1] = 0;
}

static void screen_input(int fi) {
    const Formula *f = &FORMS[fi];
    int sel = 0, k, i, bad;
    char msg[32]; msg[0] = 0;
    for (;;) {
        char line[32];
        Bdisp_AllClr_VRAM();
        pr(1, f->cat, 1);
        pr(2, f->desc, 0);
        pr(3, f->form, 0);
        for (i = 0; i < f->nv; i++) {
            int j = 0, p = 0;
            line[p++] = ' ';
            while (f->vn[i][j]) line[p++] = f->vn[i][j++];
            line[p++] = ' '; line[p++] = '='; line[p++] = ' ';
            j = 0; while (vbuf[fi][i][j]) line[p++] = vbuf[fi][i][j++];
            if (i == sel) line[p++] = '_';
            line[p] = 0;
            pr(4 + i, line, i == sel);
        }
        if (msg[0]) pr(7, msg, 0);
        else { line[0] = 0; { int j = 0, p = 0; const char *d = f->vd[sel];
               while (d[j] && p < COLS) line[p++] = d[j++]; line[p] = 0; } pr(7, line, 0); }
        pr(8, "EXE:calcular EXIT:atras", 1);
        Bdisp_PutDisp_DD();

        k = getk(); msg[0] = 0;
        if (k == KEY_CTRL_EXIT) return;
        if (k == KEY_CTRL_UP)   { sel = (sel + f->nv - 1) % f->nv; continue; }
        if (k == KEY_CTRL_DOWN) { sel = (sel + 1) % f->nv; continue; }
        if (k == KEY_CTRL_DEL) { int n = c_len(vbuf[fi][sel]); if (n) vbuf[fi][sel][n - 1] = 0; continue; }
        if (k == KEY_CTRL_AC)  { vbuf[fi][sel][0] = 0; continue; }
        if (k == KEY_CTRL_EXE) {
            double v[3] = {0, 0, 0}; bad = -1;
            for (i = 0; i < f->nv; i++)
                if (!c_parse(vbuf[fi][i], &v[i])) { bad = i; break; }
            if (bad >= 0) {
                int p = 0, j = 0;
                const char *t = "Dato invalido: ";
                while (t[j]) msg[p++] = t[j++];
                j = 0; while (f->vn[bad][j]) msg[p++] = f->vn[bad][j++];
                msg[p] = 0; sel = bad;
                continue;
            }
            f->fn(v, &out);
            screen_result();
            continue;
        }
        { char c = key_to_char(k); if (c) edit_append(vbuf[fi][sel], c); }
    }
}

/* ---------- menu principal ---------- */
int main(void) {
    int sel = 0, top = 0, k, i;
    const int vis = 5;
    for (;;) {
        char line[32];
        if (sel < top) top = sel;
        if (sel >= top + vis) top = sel - vis + 1;
        Bdisp_AllClr_VRAM();
        pr(1, FORMS[sel].cat, 1);
        pr(2, FORMS[sel].desc, 0);
        pr(3, "---------------------", 0);
        for (i = 0; i < vis && top + i < NFORM; i++) {
            int idx = top + i, p = 0, j = 0;
            if (idx + 1 >= 10) line[p++] = '0' + (idx + 1) / 10;
            line[p++] = '0' + (idx + 1) % 10;
            line[p++] = ' ';
            while (FORMS[idx].form[j] && p < COLS) line[p++] = FORMS[idx].form[j++];
            line[p] = 0;
            pr(4 + i, line, idx == sel);
        }
        Bdisp_PutDisp_DD();
        k = getk();
        if (k == KEY_CTRL_UP)   sel = (sel + NFORM - 1) % NFORM;
        if (k == KEY_CTRL_DOWN) sel = (sel + 1) % NFORM;
        if (k == KEY_CTRL_EXE)  screen_input(sel);
        if (k == KEY_CTRL_EXIT) return 0;
    }
}
