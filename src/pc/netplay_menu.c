/* The multiplayer menus share SM64's HUD font, dialog font, boxes and cursor. */
#ifdef __ANDROID__
#include "sm64.h"
#include "game/game_init.h"
#include "game/ingame_menu.h"
#include "game/segment2.h"
#include "game/memory.h"
#include "engine/math_util.h"
#include "android_feedback.h"
#include "netplay_lan.h"
#include "netplay_menu.h"
#include <string.h>

static struct { int page, selected, status, ready, frame; char detail[256]; } menu;
static float cursorY;
static int previousPage = -1;

void lan_menu_set_state(int page, int selected, int status, int ready, const char *detail) {
    menu.page = page; menu.selected = selected; menu.status = status; menu.ready = ready;
    strncpy(menu.detail, detail ? detail : "", sizeof(menu.detail) - 1);
    menu.detail[sizeof(menu.detail) - 1] = 0;
}
static void encode(const char *ascii, u8 *text, int max) {
    int n;
    for (n = 0; n < max - 1 && ascii[n]; ++n) {
        char c = ascii[n];
        text[n] = (c == ' ' || c == '-' || c == '_') ? DIALOG_CHAR_SPACE : c == '.' ? DIALOG_CHAR_PERIOD
                : c == '/' ? DIALOG_CHAR_SLASH : ASCII_TO_DIALOG(c);
    }
    text[n] = DIALOG_CHAR_TERMINATOR;
}
static void text(int x, int y, const char *label, int selected, int alpha) {
    u8 chars[96]; encode(label, chars, sizeof(chars));
    gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, alpha);
    print_generic_string(x + 1, y - 1, chars);
    gDPSetEnvColor(gDisplayListHead++, selected ? 255 : 210, selected ? 240 : 210, selected ? 160 : 210, alpha);
    print_generic_string(x, y, chars);
    gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
}
static void box(int x, int top, int width, int height, int alpha) {
    create_dl_translation_matrix(MENU_MTX_PUSH, x, top, 0);
    create_dl_scale_matrix(MENU_MTX_NOPUSH, width / 130.0f, height / 80.0f, 1);
    gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, alpha);
    gSPDisplayList(gDisplayListHead++, dl_draw_text_bg_box);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}
static void title(const char *label, int y, int alpha) {
    u8 chars[40]; encode(label, chars, sizeof(chars));
    int width = (int)strlen(label) * 12;
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_begin);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, alpha);
    print_hud_lut_string(HUD_LUT_GLOBAL, 160 - width / 2, 224 - y, chars);
    gSPDisplayList(gDisplayListHead++, dl_rgba16_text_end);
}
static void vibration(int y, int alpha) {
    int enabled = android_rumble_enabled();
    box(66, y + 19, 188, 26, alpha * 2 / 5);
    box(enabled ? 164 : 204, y + 17, enabled ? 34 : 42, 22, alpha * 3 / 4);
    text(82, y, "Vibrazione", 1, alpha);
    text(171, y, "ON", enabled, enabled ? alpha : alpha * 2 / 3);
    text(210, y, "OFF", !enabled, enabled ? alpha * 2 / 3 : alpha);
}
static const char *status_text(void) {
    const char *d = menu.detail;
    if (menu.status == LAN_NET_PAIRED)
        return menu.ready & 1 ? "Pronto. Attendi il compagno." : menu.ready & 2 ? "Il compagno e pronto." : "Confermate entrambi per giocare.";
    if (menu.status == LAN_NET_RECONNECTING) return "Riconnessione in corso...";
    if (strstr(d, "PERMISSION DENIED")) return "Permesso Wi Fi negato.";
    if (strstr(d, "PERMISSION")) return "Consenti il permesso Wi Fi.";
    if (strstr(d, "LOCATION")) return "Accendi Wi Fi e localizzazione.";
    if (strstr(d, "ACCEPT")) return "Accetta sull altro telefono.";
    if (strstr(d, "NOT SUPPORTED")) return "Usa una rete Wi Fi o hotspot.";
    if (strstr(d, "WAITING")) return "In attesa di Luigi...";
    if (strstr(d, "CREATING")) return "Creazione partita vicina...";
    if (menu.status == LAN_NET_ERROR) return "Connessione persa. Riprova.";
    return "Stessa rete Wi Fi o hotspot.";
}
void lan_menu_render(void) {
    if (!menu.page) { previousPage = 0; return; }
    create_dl_ortho_matrix();
    if (previousPage != menu.page) { menu.frame = 0; previousPage = menu.page; cursorY = 112; }
    if (menu.frame < 8) ++menu.frame;
    float t = menu.frame / 8.0f;
    float ease = t * t * (3 - 2 * t);
    int alpha = (int)(255 * ease), slide = (int)(8 * (1 - ease));
    if (menu.page == 5) {
        vibration(202, 255);
        text(92, 186, "L cambia / tocca ON o OFF", 0, 225);
        return;
    }
    title(menu.page == 1 ? "GIOCA" : menu.page == 4 ? "CONNESSIONE" : "MULTIPLAYER", 144 + slide, alpha);
    box(66, 132 + slide, 188, menu.page == 6 ? 88 : 62, alpha / 3);
    const char *first = menu.page == 1 ? "Giocatore singolo" : menu.page == 4 ? "Wi Fi / Hotspot"
                      : menu.page == 2 ? "Crea partita" : menu.status == LAN_NET_PAIRED ? "Sono pronto" : "Riprova";
    const char *second = menu.page == 1 ? "Multiplayer" : menu.page == 4 ? "Wi Fi Direct"
                       : menu.page == 2 ? "Cerca partita" : "Indietro";
    if (menu.page == 6) {
        char names[256]; strncpy(names, menu.detail, sizeof(names)); names[255] = 0;
        char *line = names;
        int index = 0, start = menu.selected / 3 * 3;
        while (line && *line) {
            char *end = strchr(line, '\n'); if (end) *end = 0;
            if (index >= start && index < start + 3) text(92, 112 - (index - start) * 26 + slide, line, menu.selected == index, alpha);
            ++index; line = end ? end + 1 : NULL;
        }
    } else {
        if (menu.page == 3 && menu.status != LAN_NET_PAIRED && menu.status != LAN_NET_ERROR) first = "Ricerca in corso...";
        text(94, 112 + slide, first, menu.selected == 0, alpha);
        text(94, 86 + slide, second, menu.selected == 1, alpha);
        vibration(48 + slide, alpha);
    }
    if (menu.page == 3) text(68, 68 + slide, status_text(), 0, alpha);
    float target = menu.page == 6 ? 112 - (menu.selected % 3) * 26 : menu.selected == 2 ? 48 : 112 - menu.selected * 26;
    cursorY += (target - cursorY) * .45f;
    create_dl_translation_matrix(MENU_MTX_PUSH, 74 + sins(gGlobalTimer * 0x600) * 2, cursorY + 3 + slide, 0);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, alpha);
    gSPDisplayList(gDisplayListHead++, dl_draw_triangle);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
    text(69, 19, "A / START scegli", 0, alpha);
    if (menu.page != 1) text(204, 19, "B indietro", 0, alpha);
}
#else
#include "netplay_menu.h"
void lan_menu_set_state(int page, int selected, int status, int ready, const char *detail) {
    (void)page; (void)selected; (void)status; (void)ready; (void)detail;
}
void lan_menu_render(void) {}
#endif
