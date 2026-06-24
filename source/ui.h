/*
 * ui.h - citro2d-based UI primitives.
 *
 * All rendering goes through frame_begin/frame_end. citro2d handles the
 * GPU pipeline (C3D_FrameBegin, C2D_TargetClear, C3D_FrameEnd) so the
 * render target is fully cleared each frame — no ghosting.
 *
 * Coordinates are float pixels on the top screen (400x240).
 */
#ifndef UI_H
#define UI_H

#include <citro2d.h>
#include <citro3d.h>
#include <3ds.h>
#include <stdbool.h>
#include <stdint.h>

/* Top screen geometry. */
#define TOP_W        400
#define TOP_H        240
#define BOT_W        320
#define BOT_H        240
#define LINE_H        16
#define LINE_H_TIGHT  14
#define MARGIN_X       6
#define MARGIN_Y       6
#define CONTENT_Y_TOP   24
#define CONTENT_Y_BOT  212

/* Colour palette (ARGB8888). */
extern const u32 COL_BG;
extern const u32 COL_TITLEBG;
extern const u32 COL_TITLEFG;
extern const u32 COL_TEXT;
extern const u32 COL_HILITE;
extern const u32 COL_HIFG;
extern const u32 COL_DIM;
extern const u32 COL_OK;
extern const u32 COL_ERR;
extern const u32 COL_WARN;
extern const u32 COL_FOOTBG;
extern const u32 COL_FOOTFG;

/* Initialise gfx + citro2d + text buffer. Returns false on failure. */
bool ui_init(void);
void ui_fini(void);

/* Per-frame lifecycle. frame_begin clears both targets, prepares text,
 * and scenes the TOP target by default. Call scene_bottom() to switch
 * to the bottom screen mid-frame and back via scene_top(). */
void frame_begin(void);
void frame_end(void);
void scene_top(void);
void scene_bottom(void);

/* Convenience bottom-screen header/footer. Same vertical metrics as the
 * top-screen versions but laid out for 320 px wide. */
void draw_title_bot(const char *t);
void draw_footer_bot(const char *hint);

/* Drawing primitives. `size` ~ 0.4..0.6 for typical body text. */
void draw_text(float x, float y, float size, u32 col, const char *s);
__attribute__((format(printf, 5, 6)))
void draw_textf(float x, float y, float size, u32 col, const char *fmt, ...);
void draw_rect(float x, float y, float w, float h, u32 col);

/* Hollow rectangle of thickness `t` (border only, no fill). Use for
 * selection markers so an icon drawn inside isn't tinted by a fill
 * showing through its transparent pixels. */
void draw_frame(float x, float y, float w, float h, float t, u32 col);

/* Header / footer bars. */
void draw_title(const char *t);
void draw_footer(const char *hint);

/* Block until any key in `mask` is pressed; redraws nothing — the caller
 * is responsible for having drawn a frame before. */
void wait_for_key(u32 mask);

/* ===== Menu ============================================================ */
typedef struct {
    const char *label;
    const char *hint;
    bool        enabled;
} menu_item_t;

/* Returns selected index, -1 on B (back), -2 on START (exit). */
int run_menu(const char *title, const menu_item_t *items, int n, int initial);

/* ===== Confirmation dialog ============================================= */
bool confirm_dialog(const char *title, const char *line1, const char *line2);

/* ===== Info screen (custom draw fn, B/A to dismiss) =================== */
typedef void (*ui_draw_fn_t)(void *ctx);
void run_info_screen(const char *title, const char *footer_hint,
                     ui_draw_fn_t draw, void *ctx);

#endif
