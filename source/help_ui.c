#include "help_ui.h"

#include <3ds.h>
#include "app_version.h"
#include "ui.h"

void show_help(void) {
    int page = 0;
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown();
        if (d & (KEY_B | KEY_START | KEY_A)) return;
        if (d & (KEY_R | KEY_DRIGHT | KEY_L | KEY_DLEFT)) page = 1 - page;
        frame_begin();
        if (page == 0) {
            draw_title("Help (1/2) - workflow");
            float y = 30;
            draw_text(MARGIN_X, y, 0.45f, COL_TEXT,
                      "Full Pokewalker experience WITHOUT IR:");
            y += LINE_H + 4;
            const char *lines[] = {
                "1. Send  : write Pokemon from sav INTO pweep.rom",
                "2. Flash : pweep.rom -> walker",
                "3. Walk  : real-world steps, catches, watts",
                "4. Dump  : walker -> pweep.rom",
                "5. CLAIM : caught -> HGSS boxes + return sent",
                "           Pokemon + sync steps/watts",
                "6. Save  : copy new .sav onto your HGSS cart",
            };
            for (size_t i = 0; i < sizeof(lines)/sizeof(*lines); i++) {
                draw_text(MARGIN_X, y, 0.45f, COL_TEXT, lines[i]);
                y += LINE_H;
            }
        } else {
            draw_title("Help (2/2) - file locations");
            float y = 30;
            draw_text(MARGIN_X, y, 0.5f, COL_HIFG, "pweep.rom (64 KB):"); y += LINE_H;
            draw_text(MARGIN_X + 14, y, 0.45f, COL_TEXT, "primary  : /3ds/pokeStride"); y += LINE_H;
            draw_text(MARGIN_X + 14, y, 0.45f, COL_DIM,  "fallback : /3ds/pkwbridge"); y += LINE_H;
            draw_text(MARGIN_X + 14, y, 0.45f, COL_DIM,  "fallback : sdmc:/"); y += LINE_H + 4;
            draw_text(MARGIN_X, y, 0.5f, COL_HIFG, "HGSS .sav (512 KB):"); y += LINE_H;
            draw_text(MARGIN_X + 14, y, 0.45f, COL_TEXT, "primary  : /roms/nds/saves"); y += LINE_H;
            draw_text(MARGIN_X + 14, y, 0.45f, COL_DIM,  "fallback : /3ds/pkwbridge"); y += LINE_H;
            draw_text(MARGIN_X + 14, y, 0.45f, COL_DIM,  "fallback : sdmc:/"); y += LINE_H + 4;
            draw_text(MARGIN_X, y, 0.45f, COL_DIM,
                      "Outputs (timestamped) go next to inputs.");
            y += LINE_H + 4;
            draw_textf(MARGIN_X, y, 0.42f, COL_DIM, "%s v%s",
                       APP_NAME, APP_VERSION);
        }
        draw_footer("(L/R) page  (B) back");
        frame_end();
    }
}
