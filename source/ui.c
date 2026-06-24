#include "ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* citro2d's C2D_Color32 packs ABGR (alpha high, then BGR), so literals
 * here are 0xAABBGGRR. */
const u32 COL_BG      = 0xFF1B1B1F;
const u32 COL_TITLEBG = 0xFFFFFFFF;
const u32 COL_TITLEFG = 0xFF161A1A;
const u32 COL_TEXT    = 0xFFE0E0E0;
const u32 COL_HILITE  = 0xFFC4427A;  /* primary accent */
const u32 COL_HIFG    = 0xFFFFFFFF;
const u32 COL_DIM     = 0xFF8A8A8A;
const u32 COL_OK      = 0xFF40C840;
const u32 COL_ERR     = 0xFF5050FF;  /* red (ABGR) */
const u32 COL_WARN    = 0xFF40B7D7;  /* gold (ABGR) */
const u32 COL_FOOTBG  = 0xFF2B2B2E;
const u32 COL_FOOTFG  = 0xFFFFFFFF;

static C3D_RenderTarget *g_top    = NULL;
static C3D_RenderTarget *g_bottom = NULL;
static C2D_TextBuf       g_tbuf   = NULL;
static C2D_Font          g_font   = NULL;

bool ui_init(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    g_top    = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    g_bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    g_tbuf   = C2D_TextBufNew(4096);
    g_font   = NULL;
    return g_top && g_bottom && g_tbuf;
}

void ui_fini(void) {
    if (g_tbuf) { C2D_TextBufDelete(g_tbuf); g_tbuf = NULL; }
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void frame_begin(void) {
    C2D_TextBufClear(g_tbuf);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(g_top,    COL_BG);
    C2D_TargetClear(g_bottom, COL_BG);
    C2D_SceneBegin(g_top);
}

void frame_end(void) {
    C3D_FrameEnd(0);
}

void scene_top(void)    { C2D_SceneBegin(g_top); }
void scene_bottom(void) { C2D_SceneBegin(g_bottom); }

void draw_title_bot(const char *t) {
    draw_rect(0, 0, BOT_W, 22, COL_TITLEBG);
    draw_rect(0, 22, BOT_W, 1, COL_TEXT);
    int est_w = (int)strlen(t) * 9;
    float x = (BOT_W - est_w) / 2.0f;
    if (x < MARGIN_X) x = MARGIN_X;
    draw_text(x, 3, 0.55f, COL_TITLEFG, t);
}

void draw_footer_bot(const char *hint) {
    draw_rect(0, BOT_H - 18, BOT_W, 18, COL_FOOTBG);
    draw_rect(0, BOT_H - 19, BOT_W, 1, COL_TEXT);
    if (hint) draw_text(MARGIN_X, BOT_H - 16, 0.42f, COL_FOOTFG, hint);
}

void draw_text(float x, float y, float size, u32 col, const char *s) {
    if (!s || !*s) return;
    C2D_Text t;
    C2D_TextFontParse(&t, g_font, g_tbuf, s);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, x, y, 0.5f, size, size, col);
}

void draw_textf(float x, float y, float size, u32 col, const char *fmt, ...) {
    char buf[256];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    draw_text(x, y, size, col, buf);
}

void draw_rect(float x, float y, float w, float h, u32 col) {
    C2D_DrawRectSolid(x, y, 0.0f, w, h, col);
}

void draw_frame(float x, float y, float w, float h, float t, u32 col) {
    draw_rect(x,             y,             w, t, col);  /* top */
    draw_rect(x,             y + h - t,     w, t, col);  /* bottom */
    draw_rect(x,             y,             t, h, col);  /* left */
    draw_rect(x + w - t,     y,             t, h, col);  /* right */
}

void draw_title(const char *t) {
    draw_rect(0, 0, TOP_W, 22, COL_TITLEBG);
    draw_rect(0, 22, TOP_W, 1, COL_TEXT);
    int est_w = (int)strlen(t) * 9;
    float x = (TOP_W - est_w) / 2.0f;
    if (x < MARGIN_X) x = MARGIN_X;
    draw_text(x, 3, 0.55f, COL_TITLEFG, t);
}

void draw_footer(const char *hint) {
    draw_rect(0, TOP_H - 18, TOP_W, 18, COL_FOOTBG);
    draw_rect(0, TOP_H - 19, TOP_W, 1, COL_TEXT);
    if (hint) draw_text(MARGIN_X, TOP_H - 16, 0.45f, COL_FOOTFG, hint);
}

void wait_for_key(u32 mask) {
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & mask) return;
        frame_begin();
        frame_end();
    }
}

int run_menu(const char *title, const menu_item_t *items, int n, int initial) {
    int cursor = initial;
    if (cursor < 0 || cursor >= n) cursor = 0;
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & KEY_START) return -2;
        if (d & KEY_B)     return -1;
        if (d & KEY_DUP) {
            for (int s = 0; s < n; s++) {
                cursor = (cursor - 1 + n) % n;
                if (items[cursor].enabled) break;
            }
        }
        if (d & KEY_DDOWN) {
            for (int s = 0; s < n; s++) {
                cursor = (cursor + 1) % n;
                if (items[cursor].enabled) break;
            }
        }
        if ((d & KEY_A) && items[cursor].enabled) return cursor;

        frame_begin();
        draw_title(title);
        float y = 32;
        for (int i = 0; i < n; i++) {
            if (i == cursor) {
                draw_frame(0, y - 2, TOP_W, LINE_H + 2, 2, COL_HILITE);
                draw_text(MARGIN_X + 10, y, 0.5f, COL_HIFG, ">");
                draw_text(MARGIN_X + 30, y, 0.5f, COL_HIFG, items[i].label);
            } else {
                u32 c = items[i].enabled ? COL_TEXT : COL_DIM;
                draw_text(MARGIN_X + 30, y, 0.5f, c, items[i].label);
                if (!items[i].enabled) {
                    draw_text(TOP_W - 90, y, 0.45f, COL_DIM, "(disabled)");
                }
            }
            y += LINE_H + 4;
        }
        if (items[cursor].hint) {
            y += 6;
            draw_text(MARGIN_X, y, 0.45f, COL_DIM, items[cursor].hint);
        }
        draw_footer("(A) select   (B) back   (START) exit");
        frame_end();
    }
    return -2;
}

bool confirm_dialog(const char *title, const char *line1, const char *line2) {
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & KEY_A) return true;
        if (d & (KEY_B | KEY_START)) return false;

        frame_begin();
        draw_title(title);
        float y = 50;
        if (line1) { draw_text(MARGIN_X, y, 0.5f, COL_TEXT, line1); y += LINE_H + 4; }
        if (line2) { draw_text(MARGIN_X, y, 0.5f, COL_TEXT, line2); y += LINE_H + 4; }
        y += 20;
        draw_text(MARGIN_X, y, 0.5f, COL_OK,  "(A) confirm");  y += LINE_H + 4;
        draw_text(MARGIN_X, y, 0.5f, COL_ERR, "(B) cancel");
        draw_footer("(A) yes   (B) no");
        frame_end();
    }
    return false;
}

void run_info_screen(const char *title, const char *footer_hint,
                     ui_draw_fn_t draw, void *ctx)
{
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & (KEY_B | KEY_A | KEY_START)) return;
        frame_begin();
        draw_title(title);
        if (draw) draw(ctx);
        draw_footer(footer_hint);
        frame_end();
    }
}
