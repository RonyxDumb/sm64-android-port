/*
 * Minimal visible LAN peer for the Android multiplayer prototype.
 * Unlike full sm64coopdx, this does not replay remote physics or synchronize
 * items, stars, actors, warps, or save files.
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

static struct Object *sRemoteMario;

void lan_netplay_on_area_cleared(void) {
    /* The engine may recycle object-pool memory on area unload. */
    sRemoteMario = NULL;
}

void lan_netplay_object_tick(void) {
    struct LanPlayerState peer;
    int visible = lan_net_read_peer(&peer)
                  && gMarioObject != NULL
                  && peer.level == (u8)gCurrLevelNum
                  && peer.area == (u8)gCurrAreaIndex
                  && isfinite(peer.x) && isfinite(peer.y) && isfinite(peer.z)
                  && fabsf(peer.x) < 32768.0f && fabsf(peer.y) < 32768.0f
                  && fabsf(peer.z) < 32768.0f;

    if (sRemoteMario && !(sRemoteMario->activeFlags & ACTIVE_FLAG_ACTIVE)) {
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
        sRemoteMario = spawn_object(gMarioObject, MODEL_MARIO, bhvLanRemoteMario);
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

    /* Temporary local animation mirror; remote animation IDs will be
     * transmitted and decoded in the complete netplay implementation. */
    o->header.gfx.animInfo = gMarioObject->header.gfx.animInfo;
}
#endif
