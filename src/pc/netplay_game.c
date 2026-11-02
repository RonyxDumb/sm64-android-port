/*
 * Two-player avatar replication for Android local play.
 * Position, animation and level travel are client-owned. Session-earned stars
 * and keys are replicated separately; dynamic actors still simulate locally.
 */
#ifdef __ANDROID__
#include <math.h>
#include "sm64.h"
#include "behavior_data.h"
#include "model_ids.h"
#include "game/area.h"
#include "game/level_update.h"
#include "game/object_helpers.h"
#include "game/object_list_processor.h"
#include "netplay_lan.h"
#include "game/game_init.h"
#include "game/memory.h"
#include "game/mario.h"
#include "game/save_file.h"
#include "engine/graph_node.h"
#include <string.h>

static struct Object *sRemoteMario;
static struct MarioAnimation sAnimation;
/* A separate animation DMA target is essential: the local target is overwritten
 * whenever Mario changes animation. Never retain a pointer into that buffer. */
static union { uint64_t align; unsigned char data[0x4000]; } sAnimationBuffer;

static void remote_animation(struct Object *o, const struct LanPlayerState *peer) {
    struct MarioAnimation *local = gMarioState->animation;
    struct Animation *animation = (struct Animation *)sAnimationBuffer.data;
    struct AnimInfo *info = &o->header.gfx.animInfo;
    if (!local || !local->animDmaTable || peer->anim < 0
        || (u32)peer->anim >= local->animDmaTable->count
        || local->animDmaTable->anim[peer->anim].size > sizeof(sAnimationBuffer.data)) return;
    sAnimation.animDmaTable = local->animDmaTable;
    sAnimation.targetAnim = animation;
    if (load_patchable_table(&sAnimation, peer->anim)) {
        animation->values = (void *)((u8 *)animation + (uintptr_t)animation->values);
        animation->index = (void *)((u8 *)animation + (uintptr_t)animation->index);
    }
    info->animID = peer->anim;
    info->curAnim = animation;
    info->animFrame = peer->frame < animation->loopEnd ? peer->frame : animation->loopEnd - 1;
    if (info->animFrame < 0) info->animFrame = 0;
    info->animYTrans = peer->anim_y;
    info->animAccel = peer->anim_accel;
    info->animFrameAccelAssist = (s32)info->animFrame * 65536;
    info->animTimer = gAreaUpdateCounter;
}

void lan_netplay_on_area_cleared(void) {
    /* The engine may recycle object-pool memory on area unload. */
    sRemoteMario = NULL;
    memset(&sAnimation, 0, sizeof(sAnimation));
}

void lan_netplay_object_tick(void) {
    struct LanPlayerState peer;
    /* Host is player 1 (Mario); joining device is player 2 (Luigi), locally too. */
    if (gMarioObject && lan_net_status() == LAN_NET_PAIRED) {
        int model = lan_net_is_host() ? MODEL_MARIO : MODEL_LUIGI;
        gMarioObject->header.gfx.sharedChild = gLoadedGraphNodes[model];
    }
    if (lan_net_status() == LAN_NET_PAIRED && gCurrSaveFileNum >= 1 && gCurrSaveFileNum <= 4 && gMarioObject) {
        uint8_t stars[LAN_PROGRESS_COURSES];
        uint32_t keys;
        int i;
        lan_net_progress(stars, &keys);
        for (i = 0; i < LAN_PROGRESS_COURSES && i <= COURSE_COUNT; ++i) {
            if (stars[i] & ~save_file_get_star_flags(gCurrSaveFileNum - 1, i - 1))
                save_file_set_star_flags(gCurrSaveFileNum - 1, i - 1, stars[i]);
        }
        if ((keys & 1) && !(save_file_get_flags() & (SAVE_FLAG_HAVE_KEY_1 | SAVE_FLAG_UNLOCKED_BASEMENT_DOOR))) save_file_set_flags(SAVE_FLAG_HAVE_KEY_1);
        if ((keys & 2) && !(save_file_get_flags() & (SAVE_FLAG_HAVE_KEY_2 | SAVE_FLAG_UNLOCKED_UPSTAIRS_DOOR))) save_file_set_flags(SAVE_FLAG_HAVE_KEY_2);
        gMarioState->numStars = save_file_get_total_star_count(gCurrSaveFileNum - 1, COURSE_MIN - 1, COURSE_MAX - 1);
    }
    int visible = lan_net_read_peer(&peer)
                  && gMarioObject != NULL
                  && peer.level == (u8)gCurrLevelNum
                  && peer.area == (u8)gCurrAreaIndex
                  && isfinite(peer.x) && isfinite(peer.y) && isfinite(peer.z)
                  && fabsf(peer.x) < 32768.0f && fabsf(peer.y) < 32768.0f
                  && fabsf(peer.z) < 32768.0f;

    if (sRemoteMario && (!(sRemoteMario->activeFlags & ACTIVE_FLAG_ACTIVE)
        || sRemoteMario->behavior != bhvLanRemoteMario)) {
        sRemoteMario = NULL;
    }

    if (!visible) {
        if (sRemoteMario) {
            obj_mark_for_deletion(sRemoteMario);
            sRemoteMario = NULL;
        }
        return;
    }

    if (!sRemoteMario) {
        sRemoteMario = spawn_object(gMarioObject, lan_net_is_host() ? MODEL_LUIGI : MODEL_MARIO, bhvLanRemoteMario);
        if (sRemoteMario) {
            sRemoteMario->oPosX = peer.x;
            sRemoteMario->oPosY = peer.y;
            sRemoteMario->oPosZ = peer.z;
            sRemoteMario->oRoom = -1;
        }
    }
}

void bhv_lan_remote_mario_loop(void) {
    struct LanPlayerState peer;
    struct Object *o = gCurrentObject;
    float dx, dy, dz;

    if (!lan_net_read_peer(&peer) || gMarioObject == NULL) return;
    if (peer.level != (u8)gCurrLevelNum || peer.area != (u8)gCurrAreaIndex) return;

    dx = peer.x - o->oPosX;
    dy = peer.y - o->oPosY;
    dz = peer.z - o->oPosZ;
    if (dx * dx + dy * dy + dz * dz > 600.0f * 600.0f) {
        o->oPosX = peer.x;
        o->oPosY = peer.y;
        o->oPosZ = peer.z;
    } else {
        o->oPosX += dx * 0.34f;
        o->oPosY += dy * 0.34f;
        o->oPosZ += dz * 0.34f;
    }
    o->oFaceAngleYaw = peer.yaw;
    o->oMoveAngleYaw = peer.yaw;

    o->oFaceAnglePitch = peer.pitch;
    o->oFaceAngleRoll = peer.roll;
    remote_animation(o, &peer);
}
#endif
