/* Run the actual menu and generic text renderer without a GPU or ROM. The font
 * fixture preserves the real US LUT's null entries, not its copyrighted pixels. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/game/ingame_menu.c"
#include "../src/pc/netplay_menu.c"

static u8 texture[256];
#include "menu_font_fixture.h"
u8 *main_hud_lut[58] = { [0 ... 57] = texture };
u8 menu_hud_lut[58 * sizeof(void *)]; // JP-only table, unused by these US menus.
static Gfx commands[32768];
Gfx *gDisplayListHead;
Gfx dl_ia_text_begin[1], dl_ia_text_end[1], dl_ia_text_tex_settings[1];
Gfx dl_rgba16_text_begin[1], dl_rgba16_text_end[1], dl_rgba16_load_tex_block[1];
Gfx dl_draw_text_bg_box[1], dl_draw_triangle[1];
u32 gGlobalTimer;
f32 gSineTable[4096];
static Mtx matrices[4096];
static size_t matrixCount;
static int enabled;

int android_rumble_enabled(void) { return enabled; }
void *segmented_to_virtual(const void *address) { return (void *)address; }
void *alloc_display_list(u32 size) {
    assert(size <= sizeof(Mtx) && matrixCount < 4096);
    return &matrices[matrixCount++];
}
void guTranslate(Mtx *m, float x, float y, float z) { (void)m; (void)x; (void)y; (void)z; }
void guScale(Mtx *m, float x, float y, float z) { (void)m; (void)x; (void)y; (void)z; }
void guMtxIdent(Mtx *m) { (void)m; }
void guOrtho(Mtx *m, float l, float r, float b, float t, float n, float f, float s) {
    (void)m; (void)l; (void)r; (void)b; (void)t; (void)n; (void)f; (void)s;
}

static void render(int page, int selected, int state, int ready, const char *detail) {
    gDisplayListHead = commands;
    matrixCount = 0;
    lan_menu_set_state(page, selected, state, ready, detail);
    lan_menu_render();
    assert(gDisplayListHead < commands + 32768);
    size_t textures = 0;
    for (Gfx *cmd = commands; cmd < gDisplayListHead; ++cmd) {
        if ((cmd->words.w0 >> 24) == G_SETTIMG) {
            if (!cmd->words.w1) {
                fprintf(stderr, "NULL font texture: page=%d status=%d detail=%s\n", page, state, detail);
                assert(cmd->words.w1 != 0);
            }
            ++textures;
        }
    }
    assert(page == 0 || textures > 0);
}

int main(void) {
    const char *details[] = { "SAME WIFI OR HOTSPOT", "CREATING DIRECT GROUP",
        "WAITING FOR PLAYER TWO", "DIRECT GROUP CONNECTED", "ACCEPT ON HOST PHONE",
        "ENABLE LOCATION AND WIFI", "WIFI PERMISSION DENIED", "WIFI PERMISSION REQUIRED",
        "WIFI DIRECT NOT SUPPORTED", "WIFI DIRECT UNAVAILABLE", "DIRECT TIMEOUT TRY AGAIN" };
    assert(main_font_lut[0x3F] && !main_font_lut[DIALOG_CHAR_PERIOD_OR_HANDAKUTEN]);
    for (enabled = 0; enabled <= 1; ++enabled) {
        for (int page = 0; page <= 6; ++page)
            for (int selection = 0; selection < 3; ++selection)
                for (int frame = 0; frame < 9; ++frame)
                    render(page, selection, LAN_NET_SEARCHING, 0, page == 6 ? "Host 1\nHost 2\nHost 3" : details[0]);
        for (size_t i = 0; i < sizeof(details) / sizeof(*details); ++i) {
            render(3, 0, LAN_NET_SEARCHING, 0, details[i]);
            render(3, 0, LAN_NET_ERROR, 0, details[i]);
        }
        for (int ready = 0; ready < 4; ++ready) render(3, 0, LAN_NET_PAIRED, ready, "");
        render(3, 0, LAN_NET_RECONNECTING, 0, "");
    }
    puts("menu rendering: all pages, host/search, ready/error/reconnect, ON/OFF and real font holes PASS");
}
