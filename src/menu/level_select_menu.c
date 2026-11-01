#include <PR/ultratypes.h>

#include "audio/external.h"
#include "engine/math_util.h"
#include "game/area.h"
#include "game/game_init.h"
#include "game/level_update.h"
#include "game/main.h"
#include "game/memory.h"
#include "game/print.h"
#include "game/save_file.h"
#include "game/sound_init.h"
#include "level_table.h"
#include "seq_ids.h"
#include "sm64.h"

#ifdef __ANDROID__
#include "pc/netplay_lan.h"

/* Mode selection is confined to Android: other targets retain their title. */
static u8 sAndroidMenuPage = 1; /* 1 mode, 2 role, 3 connection, 4 transport */
static u8 sAndroidTransport = 0; /* LAN or Wi-Fi Direct */
static u8 sAndroidMode = 0;     /* 0 single, 1 multiplayer */
static u8 sAndroidRole = 0;     /* 0 host, 1 join */
static u8 sAndroidStickNeutral = 1;

/* print_text uses a 12px advance and a 16px glyph on a 320x240 canvas.
 * '-' is the HUD star glyph; '<' and '>' are unsupported and render as spaces. */
static void android_menu_option(s32 y, const char *label, u8 selected) {
    s32 length = 0;
    while (label[length]) ++length;
    print_text_centered(160, y, label);
    if (selected) {
        print_text(160 - length * 6 - 24, y, "-");
        print_text(160 + length * 6 + 8, y, "-");
    }
}

static void android_menu_footer(u8 canSelect) {
    /* Leave the bottom centre clear for the touchscreen START button. */
    if (canSelect) print_text(36, 8, "A SELECT");
    print_text(212, 8, "B BACK");
}

static void android_menu_detail(const char *text) {
    char line[25];
    s32 row, i;
    /* Keep Android error messages inside the central safe area. */
    for (row = 0; row < 2 && *text; ++row) {
        for (i = 0; i < 24 && text[i]; ++i) line[i] = text[i];
        line[i] = 0;
        text += i;
        print_text_centered(160, 70 - row * 24, line);
    }
}
#endif

#define PRESS_START_DEMO_TIMER 800

#define STUB_LEVEL(textname, _1, _2, _3, _4, _5, _6, _7, _8) textname,
#define DEFINE_LEVEL(textname, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10) textname,

static char gLevelSelect_StageNamesText[64][16] = {
    #include "levels/level_defines.h"
};
#undef STUB_LEVEL
#undef DEFINE_LEVEL

static u16 gDemoCountdown = 0;
#ifndef VERSION_JP
static s16 D_U_801A7C34 = 1;
static s16 gameOverNotPlayed = 1;
#endif

// run the demo timer on the PRESS START screen.
// this function will return a non-0 timer once
// the demo starts, signaling to the subsystem that
// the demo needs to be ran.

// don't shift this function from being the first function in the segment.
// the level scripts assume this function is the first, so it cant be moved.
s32 run_press_start_demo_timer(s32 timer) {
    gCurrDemoInput = NULL;

    if (timer == 0) {
        if (!gPlayer1Controller->buttonDown && !gPlayer1Controller->stickMag) {
            if ((++gDemoCountdown) == PRESS_START_DEMO_TIMER) {
                // start the demo. 800 frames has passed while
                // player is idle on PRESS START screen.

                // start the Mario demo animation for the demo list.
                load_patchable_table(&gDemo, gDemoInputListID);

                // if the next demo sequence ID is the count limit, reset it back to
                // the first sequence.
                if (++gDemoInputListID == gDemo.animDmaTable->count) {
                    gDemoInputListID = 0;
                }

                // add 1 (+4) to the pointer to skip the demoID.
                gCurrDemoInput = ((struct DemoInput *) gDemo.targetAnim) + 1;
                timer = (s8)((struct DemoInput *) gDemo.targetAnim)->timer;
                gCurrSaveFileNum = 1;
                gCurrActNum = 1;
            }
        } else { // activity was detected, so reset the demo countdown.
            gDemoCountdown = 0;
        }
    }
    return timer;
}

// input loop for the level select menu. updates the selected stage
// count if an input was received. signals the stage to be started
// or the level select to be exited if start or the quit combo is
// pressed.

s16 level_select_input_loop(void) {
    s32 stageChanged = FALSE;

    // perform the ID updates per each button press.
    if (gPlayer1Controller->buttonPressed & A_BUTTON) {
        ++gCurrLevelNum, stageChanged = TRUE;
    }
    if (gPlayer1Controller->buttonPressed & B_BUTTON) {
        --gCurrLevelNum, stageChanged = TRUE;
    }
    if (gPlayer1Controller->buttonPressed & U_JPAD) {
        --gCurrLevelNum, stageChanged = TRUE;
    }
    if (gPlayer1Controller->buttonPressed & D_JPAD) {
        ++gCurrLevelNum, stageChanged = TRUE;
    }
    if (gPlayer1Controller->buttonPressed & L_JPAD) {
        gCurrLevelNum -= 10, stageChanged = TRUE;
    }
    if (gPlayer1Controller->buttonPressed & R_JPAD) {
        gCurrLevelNum += 10, stageChanged = TRUE;
    }

    // if the stage was changed, play the sound for changing a stage.
    if (stageChanged) {
        play_sound(SOUND_GENERAL_LEVEL_SELECT_CHANGE, gDefaultSoundArgs);
    }

    // TODO: enum counts for the stage lists
    if (gCurrLevelNum > LEVEL_MAX) {
        gCurrLevelNum = LEVEL_MIN; // exceeded max. set to min.
    }

    if (gCurrLevelNum < LEVEL_MIN) {
        gCurrLevelNum = LEVEL_MAX; // exceeded min. set to max.
    }

    gCurrSaveFileNum = 4; // file 4 is used for level select tests
    gCurrActNum = 6;
    print_text_centered(160, 80, "SELECT STAGE");
    print_text_centered(160, 30, "PRESS START BUTTON");
    print_text_fmt_int(40, 60, "%2d", gCurrLevelNum);
    print_text(80, 60, gLevelSelect_StageNamesText[gCurrLevelNum - 1]); // print stage name

#define QUIT_LEVEL_SELECT_COMBO (Z_TRIG | START_BUTTON | L_CBUTTONS | R_CBUTTONS)

    // start being pressed signals the stage to be started. that is, unless...
    if (gPlayer1Controller->buttonPressed & START_BUTTON) {
        // ... the level select quit combo is being pressed, which uses START. If this
        // is the case, quit the menu instead.
        if (gPlayer1Controller->buttonDown == QUIT_LEVEL_SELECT_COMBO) {
            gDebugLevelSelect = FALSE;
            return -1;
        }
        play_sound(SOUND_MENU_STAR_SOUND, gDefaultSoundArgs);
        return gCurrLevelNum;
    }
    return 0;
}

s32 intro_default(void) {
    s32 sp1C = 0;

#ifndef VERSION_JP
    if (D_U_801A7C34 == 1) {
        if (gGlobalTimer < 0x81) {
            play_sound(SOUND_MARIO_HELLO, gDefaultSoundArgs);
        } else {
            play_sound(SOUND_MARIO_PRESS_START_TO_PLAY, gDefaultSoundArgs);
        }
        D_U_801A7C34 = 0;
    }
#endif

#ifdef __ANDROID__
    {
        u16 pressed = gPlayer1Controller->buttonPressed;
        s16 stickY = gPlayer1Controller->stickY;
        u8 axisMove = 0;
        if (stickY > 35 || stickY < -35) {
            if (sAndroidStickNeutral) axisMove = 1;
            sAndroidStickNeutral = 0;
        } else {
            sAndroidStickNeutral = 1;
        }

        gDemoCountdown = 0;
        if (sAndroidMenuPage == 1) {
            print_text_centered(160, 142, "SELECT GAME MODE");
            android_menu_option(105, "SINGLE PLAYER", sAndroidMode == 0);
            android_menu_option(70, "MULTIPLAYER", sAndroidMode == 1);
            android_menu_footer(1);

            if ((pressed & (U_JPAD | D_JPAD | L_JPAD | R_JPAD)) || axisMove) {
                sAndroidMode ^= 1;
                play_sound(SOUND_MENU_CHANGE_SELECT, gDefaultSoundArgs);
            }
            if (pressed & B_BUTTON) sAndroidMenuPage = 1;
            if (pressed & (A_BUTTON | START_BUTTON)) {
                play_sound(SOUND_MENU_STAR_SOUND, gDefaultSoundArgs);
                if (sAndroidMode == 0) {
                    lan_net_stop();
                    sAndroidMenuPage = 1;
                    sp1C = 100;
#ifndef VERSION_JP
                    D_U_801A7C34 = 1;
#endif
                } else {
                    sAndroidMenuPage = 4;
                }
            }
        } else if (sAndroidMenuPage == 4) {
            print_text_centered(160, 142, "LOCAL CONNECTION");
            android_menu_option(105, "WIFI OR HOTSPOT", sAndroidTransport == 0);
            android_menu_option(70, "NEARBY WIFI DIRECT", sAndroidTransport == 1);
            android_menu_footer(1);
            if ((pressed & (U_JPAD | D_JPAD)) || axisMove) sAndroidTransport ^= 1;
            if (pressed & B_BUTTON) sAndroidMenuPage = 1;
            else if (pressed & (A_BUTTON | START_BUTTON)) sAndroidMenuPage = 2;
        } else if (sAndroidMenuPage == 2) {
            print_text_centered(160, 142, "LOCAL MULTIPLAYER");
            android_menu_option(105, "CREATE GAME", sAndroidRole == 0);
            android_menu_option(70, "FIND GAME", sAndroidRole == 1);
            print_text_centered(160, 36, sAndroidTransport ? "NO ROUTER OR INTERNET" : "SAME WIFI OR HOTSPOT");
            android_menu_footer(1);
            if ((pressed & (U_JPAD | D_JPAD | L_JPAD | R_JPAD)) || axisMove) {
                sAndroidRole ^= 1;
                play_sound(SOUND_MENU_CHANGE_SELECT, gDefaultSoundArgs);
            }
            if (pressed & B_BUTTON) sAndroidMenuPage = 4;
            else if (pressed & (A_BUTTON | START_BUTTON)) {
                if (lan_net_start(sAndroidRole == 0, sAndroidTransport) == 0) {
                    sAndroidMenuPage = 3;
                    play_sound(SOUND_MENU_STAR_SOUND, gDefaultSoundArgs);
                } else {
                    sAndroidMenuPage = 3; /* Show the error and allow back. */
                }
            }
        } else {
            int status = lan_net_status();
            print_text_centered(160, 142, sAndroidRole == 0 ? "HOST  MARIO" : "PLAYER TWO  LUIGI");
            if (status == LAN_NET_PAIRED) {
                print_text_centered(160, 105, "CONNECTED");
                print_text_centered(160, 70, "BOTH PRESS START");
                if (pressed & (A_BUTTON | START_BUTTON)) lan_net_set_ready();
                if (lan_net_both_ready()) {
                    sAndroidMenuPage = 1;
                    sp1C = 100;
                }
            } else if (status == LAN_NET_ERROR) {
                print_text_centered(160, 105, "NETWORK ERROR");
                android_menu_detail(lan_net_detail());
            } else {
                print_text_centered(160, 105, "SEARCHING");
                android_menu_detail(lan_net_detail());
            }
            android_menu_footer(status == LAN_NET_PAIRED);
            if (pressed & B_BUTTON) {
                lan_net_stop();
                sAndroidMenuPage = 2;
            }
        }
    }
#else
    print_intro_text();

    if (gPlayer1Controller->buttonPressed & START_BUTTON) {
        play_sound(SOUND_MENU_STAR_SOUND, gDefaultSoundArgs);
        sp1C = 100 + gDebugLevelSelect;
#ifndef VERSION_JP
        D_U_801A7C34 = 1;
#endif
    }
#endif
    #ifdef __ANDROID__
    return sp1C; /* Mode menus must never launch an attract-mode demo. */
#else
    return run_press_start_demo_timer(sp1C);
#endif
}

s32 intro_game_over(void) {
    s32 sp1C = 0;

#ifndef VERSION_JP
    if (gameOverNotPlayed == 1) {
        play_sound(SOUND_MARIO_GAME_OVER, gDefaultSoundArgs);
        gameOverNotPlayed = 0;
    }
#endif

    print_intro_text();

    if (gPlayer1Controller->buttonPressed & START_BUTTON) {
        play_sound(SOUND_MENU_STAR_SOUND, gDefaultSoundArgs);
        sp1C = 100 + gDebugLevelSelect;
#ifndef VERSION_JP
        gameOverNotPlayed = 1;
#endif
    }
    return run_press_start_demo_timer(sp1C);
}

s32 intro_play_its_a_me_mario(void) {
    set_background_music(0, SEQ_SOUND_PLAYER, 0);
    play_sound(SOUND_MENU_COIN_ITS_A_ME_MARIO, gDefaultSoundArgs);
    return 1;
}

s32 lvl_intro_update(s16 arg1, UNUSED s32 arg2) {
    s32 retVar;

    switch (arg1) {
        case 0:
            retVar = intro_play_its_a_me_mario();
            break;
        case 1:
            retVar = intro_default();
            break;
        case 2:
            retVar = intro_game_over();
            break;
        case 3:
            retVar = level_select_input_loop();
            break;
    }
    return retVar;
}
