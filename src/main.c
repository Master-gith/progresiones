/* Progresiones P.A. / P.G. para Casio fx-CG50 (add-in .g3a)
 * Usa libfxcg (PrizmSDK). Dibujo directo en VRAM (ver gfx.h / ui.h). */
#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include "ui.h"

static char vbuf[NFORM][3][VLEN + 1];   /* los valores se conservan por formula */
static Out  out;

static int getk(void) { int k; GetKey(&k); return k; }
static void show(void) { Bdisp_PutDisp_DD(); }

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

static void screen_result(int fi) {
    int top = 0, k;
    for (;;) {
        int shown;
        draw_result(&out, top, fi); show();
        shown = res_layout(&out, top, 0);
        k = getk();
        if (k == KEY_CTRL_EXIT || k == KEY_CTRL_EXE) return;
        if (k == KEY_CTRL_DOWN && top + shown < out.n) top++;
        if (k == KEY_CTRL_UP && top > 0) top--;
    }
}

static void screen_input(int fi) {
    const Formula *f = &FORMS[fi];
    int sel = 0, k, i;
    char msg[48]; msg[0] = 0;
    for (;;) {
        draw_input(fi, sel, msg, vbuf[fi]); show();
        k = getk(); msg[0] = 0;
        if (k == KEY_CTRL_EXIT) return;
        if (k == KEY_CTRL_UP)   { sel = (sel + f->nv - 1) % f->nv; continue; }
        if (k == KEY_CTRL_DOWN) { sel = (sel + 1) % f->nv; continue; }
        if (k == KEY_CTRL_DEL) { int n = c_len(vbuf[fi][sel]); if (n) vbuf[fi][sel][n - 1] = 0; continue; }
        if (k == KEY_CTRL_AC)  { vbuf[fi][sel][0] = 0; continue; }
        if (k == KEY_CTRL_EXE) {
            double v[3] = {0, 0, 0}; int bad = -1;
            for (i = 0; i < f->nv; i++)
                if (!c_parse(vbuf[fi][i], &v[i])) { bad = i; break; }
            if (bad >= 0) {
                /* "Dato inv\xC3\xA1lido o vac\xC3\xADo" */
                const char *t = "Falta un dato o no es v\xC3\xA1lido";
                int p = 0; while (t[p] && p < 46) { msg[p] = t[p]; p++; }
                msg[p] = 0; sel = bad;
                continue;
            }
            f->fn(v, &out);
            screen_result(fi);
            continue;
        }
        { char c = key_to_char(k); if (c) edit_append(vbuf[fi][sel], c); }
    }
}

int main(void) {
    int sel = 0, top = 0, k;
    g_vram = (unsigned short *)GetVRAMAddress();
    for (;;) {
        if (sel < top) top = sel;
        if (sel >= top + 6) top = sel - 5;
        draw_menu(sel, top); show();
        k = getk();
        if (k == KEY_CTRL_UP)   sel = (sel + NFORM - 1) % NFORM;
        if (k == KEY_CTRL_DOWN) sel = (sel + 1) % NFORM;
        if (k == KEY_CTRL_EXE)  screen_input(sel);
        if (k == KEY_CTRL_EXIT) return 0;
    }
}
