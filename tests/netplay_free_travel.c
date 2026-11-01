/* Regression: a guest must be able to select an act while its host is elsewhere. */
#include <assert.h>
#include <stdio.h>
#include "pc/netplay_lan.h"
#include "../src/menu/star_select.c"

static struct Controller controller;
struct Controller *gPlayer3Controller = &controller;
s16 gCurrLevelNum;
s8 gDialogCourseActNum;
f32 gDefaultSoundArgs[3];
static int object_updates;
void area_update_objects(void) { ++object_updates; }
void play_sound(s32 sound, f32 *position) { (void)sound; (void)position; }
/* These model the old blocking condition and remain deliberately available:
 * the selector must not depend on the peer's level or mission. */
int lan_net_status(void) { return LAN_NET_PAIRED; }
int lan_net_is_host(void) { return 0; }
int lan_net_read_peer(struct LanPlayerState *peer) {
    *peer = (struct LanPlayerState){ .level = LEVEL_CASTLE, .area = 1, .act = 1 };
    return 1;
}
int main(void) {
    gCurrLevelNum = LEVEL_BOB;
    sActSelectorMenuTimer = 11;
    sLoadedActNum = 0;
    sInitSelectedActNum = 6;
    sSelectedActIndex = 2;
    controller.buttonPressed = A_BUTTON;
    assert(lvl_update_obj_and_load_act_button_actions(0, 0) == 3);
    assert(gDialogCourseActNum == 3 && object_updates == 1);
    sLoadedActNum = 0;
    sSelectedActIndex = 4;
    controller.buttonPressed = START_BUTTON;
    assert(lvl_update_obj_and_load_act_button_actions(0, 0) == 5);
    assert(object_updates == 2);
    puts("free travel: guest A/START select its own mission while host is in castle PASS");
    return 0;
}
