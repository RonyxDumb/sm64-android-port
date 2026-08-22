#include <PR/ultratypes.h>
#include <math.h>

#include "game/memory.h"
#include "game/segment2.h"
#include "game/segment7.h"
#include "intro_geo.h"
#include "sm64.h"
#include "textures.h"
#include "types.h"
#include "prevent_bss_reordering.h"
#include "pc/gfx/gfx_pc.h"

// frame counts for the zoom in, hold, and zoom out of title model
#define INTRO_STEPS_ZOOM_IN 20
#define INTRO_STEPS_HOLD_1 75
#define INTRO_STEPS_ZOOM_OUT 91

// background types
#define INTRO_BACKGROUND_SUPER_MARIO 0
#define INTRO_BACKGROUND_GAME_OVER 1

// Original backdrop layout: 4 columns x 3 rows of 80x80 tiles.
#define INTRO_BACKDROP_TILE_SIZE 80
#define INTRO_BACKDROP_COLUMNS 4
#define INTRO_BACKDROP_ROWS 3

struct GraphNodeMore {
    /*0x00*/ struct GraphNode node;
    /*0x14*/ void *todo;
    /*0x18*/ u32 unk18;
};

// intro geo bss
s32 gGameOverFrameCounter;
s32 gGameOverTableIndex;
s16 gTitleZoomCounter;
s32 gTitleFadeCounter;

// intro screen background display lists for each of four 80x20 textures
const Gfx *introBackgroundDlRows[] = {
    title_screen_bg_dl_0A000130,
    title_screen_bg_dl_0A000148,
    title_screen_bg_dl_0A000160,
    title_screen_bg_dl_0A000178,
};

// intro screen background texture X offsets
float introBackgroundOffsetX[] = {
    0.0f, 80.0f, 160.0f, 240.0f,
    0.0f, 80.0f, 160.0f, 240.0f,
    0.0f, 80.0f, 160.0f, 240.0f,
};

// intro screen background texture Y offsets
float introBackgroundOffsetY[] = {
    160.0f, 160.0f, 160.0f, 160.0f,
    80.0f, 80.0f, 80.0f, 80.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
};

// table that points to either the "Super Mario 64" or "Game Over" tables
const u8 *const *introBackgroundTextureType[] = {
    mario_title_texture_table,
    game_over_texture_table,
};

s8 introBackgroundIndexTable[] = {
    INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO,
    INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO,
    INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO,
    INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO, INTRO_BACKGROUND_SUPER_MARIO,
};

// only one table of indexes listed
s8 *introBackgroundTables[] = { introBackgroundIndexTable };

s8 gameOverBackgroundTable[] = {
    INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER,
    INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER,
    INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER,
    INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER, INTRO_BACKGROUND_GAME_OVER,
};

// order of tiles that are flipped from "Game Over" to "Super Mario 64"
s8 gameOverBackgroundFlipOrder[] = {
    0x00, 0x01, 0x02, 0x03,
    0x07, 0x0B, 0x0A, 0x09,
    0x08, 0x04, 0x05, 0x06,
};

Gfx *geo_title_screen(s32 sp50, struct GraphNode *sp54, UNUSED void *context) {
    struct GraphNode *graphNode; // sp4c
    Gfx *displayList;            // sp48
    Gfx *displayListIter;        // sp44
    Mtx *scaleMat;               // sp40
    f32 *scaleTable1;            // sp3c
    f32 *scaleTable2;            // sp38
    f32 scaleX;                  // sp34
    f32 scaleY;                  // sp30
    f32 scaleZ;                  // sp2c

    graphNode = sp54;
    displayList = NULL;
    displayListIter = NULL;
    scaleTable1 = segmented_to_virtual(intro_seg7_table_0700C790);
    scaleTable2 = segmented_to_virtual(intro_seg7_table_0700C880);

    if (sp50 != 1) {
        gTitleZoomCounter = 0;
    } else {
        graphNode->flags = (graphNode->flags & 0xFF) | 0x100;
        scaleMat = alloc_display_list(sizeof(*scaleMat));
        displayList = alloc_display_list(4 * sizeof(*displayList));
        displayListIter = displayList;

        if (gTitleZoomCounter >= 0 && gTitleZoomCounter < INTRO_STEPS_ZOOM_IN) {
            scaleX = scaleTable1[gTitleZoomCounter * 3];
            scaleY = scaleTable1[gTitleZoomCounter * 3 + 1];
            scaleZ = scaleTable1[gTitleZoomCounter * 3 + 2];
        } else if (gTitleZoomCounter >= INTRO_STEPS_ZOOM_IN && gTitleZoomCounter < INTRO_STEPS_HOLD_1) {
            scaleX = 1.0f;
            scaleY = 1.0f;
            scaleZ = 1.0f;
        } else if (gTitleZoomCounter >= INTRO_STEPS_HOLD_1 && gTitleZoomCounter < INTRO_STEPS_ZOOM_OUT) {
            scaleX = scaleTable2[(gTitleZoomCounter - INTRO_STEPS_HOLD_1) * 3];
            scaleY = scaleTable2[(gTitleZoomCounter - INTRO_STEPS_HOLD_1) * 3 + 1];
            scaleZ = scaleTable2[(gTitleZoomCounter - INTRO_STEPS_HOLD_1) * 3 + 2];
        } else {
            scaleX = 0.0f;
            scaleY = 0.0f;
            scaleZ = 0.0f;
        }

        guScale(scaleMat, scaleX, scaleY, scaleZ);
        gSPMatrix(displayListIter++, scaleMat, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
        gSPDisplayList(displayListIter++, &intro_seg7_dl_0700B3A0);
        gSPPopMatrix(displayListIter++, G_MTX_MODELVIEW);
        gSPEndDisplayList(displayListIter);
        gTitleZoomCounter++;
    }

    return displayList;
}

Gfx *geo_fade_transition(s32 sp40, struct GraphNode *sp44, UNUSED void *context) {
    struct GraphNode *graphNode = sp44; // sp3c
    Gfx *displayList = NULL;            // sp38
    Gfx *displayListIter = NULL;        // sp34

    if (sp40 != 1) {
        gTitleFadeCounter = 0; // D_801B985C
    } else {
        displayList = alloc_display_list(5 * sizeof(*displayList));
        displayListIter = displayList;

        gSPDisplayList(displayListIter++, dl_proj_mtx_fullscreen);
        gDPSetEnvColor(displayListIter++, 255, 255, 255, gTitleFadeCounter);

        if (gTitleFadeCounter == 255) {
            graphNode->flags = (graphNode->flags & 0xFF) | 0x100;
            gDPSetRenderMode(displayListIter++, G_RM_AA_OPA_SURF, G_RM_AA_OPA_SURF2);
        } else {
            graphNode->flags = (graphNode->flags & 0xFF) | 0x500;
            gDPSetRenderMode(displayListIter++, G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2);
        }

        gSPDisplayList(displayListIter++, &intro_seg7_dl_0700C6A0);
        gSPEndDisplayList(displayListIter);

        if (gTitleZoomCounter >= 0x13) {
            gTitleFadeCounter += 0x1A;
            if (gTitleFadeCounter >= 0x100) {
                gTitleFadeCounter = 0xFF;
            }
        }
    }

    return displayList;
}

/*
 * Draw one original 80x80 title-screen tile at an arbitrary virtual-screen
 * position. Each tile is composed of four 80x20 texture strips.
 */
static Gfx *intro_backdrop_one_image_at(s32 index, s8 *backgroundTable, f32 x, f32 y) {
    Mtx *mtx;
    Gfx *displayList;
    Gfx *displayListIter;
    const u8 *const *vIntroBgTable;
    s32 i;

    mtx = alloc_display_list(sizeof(*mtx));
    displayList = alloc_display_list(36 * sizeof(*displayList));

    if (mtx == NULL || displayList == NULL) {
        return NULL;
    }

    displayListIter = displayList;
    vIntroBgTable = segmented_to_virtual(introBackgroundTextureType[backgroundTable[index]]);

    guTranslate(mtx, x, y, 0.0f);
    gSPMatrix(displayListIter++, mtx, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPDisplayList(displayListIter++, &title_screen_bg_dl_0A000118);

    for (i = 0; i < 4; ++i) {
        gDPLoadTextureBlock(displayListIter++, vIntroBgTable[i], G_IM_FMT_RGBA, G_IM_SIZ_16b,
                            80, 20, 0, G_TX_CLAMP, G_TX_CLAMP, 7, 6,
                            G_TX_NOLOD, G_TX_NOLOD)
        gSPDisplayList(displayListIter++, introBackgroundDlRows[i]);
    }

    gSPPopMatrix(displayListIter++, G_MTX_MODELVIEW);
    gSPEndDisplayList(displayListIter);

    return displayList;
}

/*
 * Keep the original helper for compatibility with any code that still calls
 * it using the classic 4x3 backdrop indexes.
 */
Gfx *intro_backdrop_one_image(s32 index, s8 *backgroundTable) {
    return intro_backdrop_one_image_at(index, backgroundTable,
                                       introBackgroundOffsetX[index],
                                       introBackgroundOffsetY[index]);
}

/* C integer division truncates toward zero, so handle negative X explicitly. */
static s32 intro_backdrop_floor_column(s32 x) {
    if (x >= 0) {
        return x / INTRO_BACKDROP_TILE_SIZE;
    }

    return -(((-x) + INTRO_BACKDROP_TILE_SIZE - 1) / INTRO_BACKDROP_TILE_SIZE);
}

/* Repeat the original four-column pattern outside the classic 320px area. */
static s32 intro_backdrop_wrapped_column(s32 column) {
    s32 wrapped = column % INTRO_BACKDROP_COLUMNS;

    if (wrapped < 0) {
        wrapped += INTRO_BACKDROP_COLUMNS;
    }

    return wrapped;
}

/*
 * Determine which 80px columns are visible for the current aspect ratio.
 * SM64 keeps a 320x240 virtual coordinate system; widescreen extends X beyond
 * 0..320 while keeping the vertical 240px range unchanged.
 */
static void intro_backdrop_get_horizontal_range(s32 *firstColumn, s32 *lastColumn) {
    f32 aspectRatio;
    f32 halfWidth;
    s32 leftEdge;
    s32 rightEdge;

    aspectRatio = gfx_current_dimensions.aspect_ratio;

    // Defensive fallback during very early startup or an invalid video mode.
    if (aspectRatio <= 0.0f) {
        aspectRatio = 4.0f / 3.0f;
    }

    halfWidth = (SCREEN_HEIGHT * aspectRatio) * 0.5f;
    leftEdge = (s32) floorf((SCREEN_WIDTH * 0.5f) - halfWidth);
    rightEdge = (s32) ceilf((SCREEN_WIDTH * 0.5f) + halfWidth);

    *firstColumn = intro_backdrop_floor_column(leftEdge);
    *lastColumn = intro_backdrop_floor_column(rightEdge - 1);
}

/*
 * Build the complete fullscreen backdrop. The original 4x3 table is repeated
 * horizontally as many times as needed to cover 16:9, 20:9 and other ratios.
 */
static Gfx *intro_backdrop_fullscreen(s8 *backgroundTable) {
    Gfx *displayList;
    Gfx *displayListIter;
    Gfx *tileDisplayList;
    s32 firstColumn;
    s32 lastColumn;
    s32 columnCount;
    s32 tileCount;
    s32 row;
    s32 column;
    s32 wrappedColumn;
    s32 tableIndex;

    intro_backdrop_get_horizontal_range(&firstColumn, &lastColumn);

    columnCount = lastColumn - firstColumn + 1;
    tileCount = columnCount * INTRO_BACKDROP_ROWS;

    // Projection + setup + one call per tile + teardown + end.
    displayList = alloc_display_list((tileCount + 4) * sizeof(*displayList));
    if (displayList == NULL) {
        return NULL;
    }

    displayListIter = displayList;

    gSPDisplayList(displayListIter++, &dl_proj_mtx_fullscreen);
    gSPDisplayList(displayListIter++, &title_screen_bg_dl_0A000100);

    for (row = 0; row < INTRO_BACKDROP_ROWS; ++row) {
        for (column = firstColumn; column <= lastColumn; ++column) {
            wrappedColumn = intro_backdrop_wrapped_column(column);
            tableIndex = row * INTRO_BACKDROP_COLUMNS + wrappedColumn;

            tileDisplayList = intro_backdrop_one_image_at(
                tableIndex,
                backgroundTable,
                (f32) (column * INTRO_BACKDROP_TILE_SIZE),
                (f32) ((INTRO_BACKDROP_ROWS - 1 - row) * INTRO_BACKDROP_TILE_SIZE)
            );

            if (tileDisplayList != NULL) {
                gSPDisplayList(displayListIter++, tileDisplayList);
            }
        }
    }

    gSPDisplayList(displayListIter++, &title_screen_bg_dl_0A000190);
    gSPEndDisplayList(displayListIter);

    return displayList;
}

Gfx *geo_intro_backdrop(s32 sp48, struct GraphNode *sp4c, UNUSED void *context) {
    struct GraphNodeMore *graphNode;
    s32 index;
    s8 *backgroundTable;
    Gfx *displayList;

    graphNode = (struct GraphNodeMore *) sp4c;
    index = graphNode->unk18 & 0xFF;
    backgroundTable = introBackgroundTables[index];
    displayList = NULL;

    if (sp48 == 1) {
        graphNode->node.flags = (graphNode->node.flags & 0xFF) | 0x100;
        displayList = intro_backdrop_fullscreen(backgroundTable);
    }

    return displayList;
}

Gfx *geo_game_over_tile(s32 sp40, struct GraphNode *sp44, UNUSED void *context) {
    struct GraphNode *graphNode;
    Gfx *displayList;
    s32 i;

    graphNode = sp44;
    displayList = NULL;

    if (sp40 != 1) {
        gGameOverFrameCounter = 0;
        gGameOverTableIndex = -2;

        for (i = 0; i < (s32) sizeof(gameOverBackgroundTable); ++i) {
            gameOverBackgroundTable[i] = INTRO_BACKGROUND_GAME_OVER;
        }
    } else {
        if (gGameOverTableIndex == -2) {
            if (gGameOverFrameCounter == 180) {
                gGameOverTableIndex++;
                gGameOverFrameCounter = 0;
            }
        } else {
            // transition tile from "Game Over" to "Super Mario 64"
            if (gGameOverTableIndex != 11 && !(gGameOverFrameCounter & 0x1)) {
                gGameOverTableIndex++;
                gameOverBackgroundTable[gameOverBackgroundFlipOrder[gGameOverTableIndex]] =
                    INTRO_BACKGROUND_SUPER_MARIO;
            }
        }

        if (gGameOverTableIndex != 11) {
            gGameOverFrameCounter++;
        }

        graphNode->flags = (graphNode->flags & 0xFF) | 0x100;
        displayList = intro_backdrop_fullscreen(gameOverBackgroundTable);
    }

    return displayList;
}