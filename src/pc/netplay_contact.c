#include "netplay_contact.h"
#include <math.h>

static float clamp(float n, float min, float max) { return n < min ? min : n > max ? max : n; }

int lan_contact_solve(const struct LanPlayerState *a, const struct LanPlayerState *b,
                      float start_x, float start_z, int host, float *dx, float *dz) {
    const float radius = 78.0f; // two original Mario cylinders, plus 4 units
    float x = a->x - b->x, z = a->z - b->z;
    float ah = a->contact & LAN_PLAYER_SHORT ? 100 : 160;
    float bh = b->contact & LAN_PLAYER_SHORT ? 100 : 160;
    *dx = *dz = 0;
    if (!(a->contact & b->contact & LAN_PLAYER_SOLID) || a->level != b->level || a->area != b->area
        || a->y + ah <= b->y + 4 || b->y + bh <= a->y + 4) return 0;
    if (!isfinite(x) || !isfinite(z) || !isfinite(a->vx) || !isfinite(a->vz)
        || !isfinite(b->vx) || !isfinite(b->vz)) return 0;
    float sx = start_x - b->x, sz = start_z - b->z;
    float step_x = x - sx, step_z = z - sz, step2 = step_x * step_x + step_z * step_z;
    float start2 = sx * sx + sz * sz;
    if (start2 > .01f && start2 < radius * radius && sx * x + sz * z < 0 && step2 < 200.0f * 200.0f) {
        float start_distance = sqrtf(start2);
        *dx = b->x + sx / start_distance * radius - a->x;
        *dz = b->z + sz / start_distance * radius - a->z;
        return 1;
    }
    // Swept cylinder test stops a dash from crossing through the other player.
    if (step2 > 0.01f && step2 < 200.0f * 200.0f && sx * sx + sz * sz >= radius * radius) {
        float dot = sx * step_x + sz * step_z;
        float disc = dot * dot - step2 * (sx * sx + sz * sz - radius * radius);
        if (disc > 0 && (sx * x + sz * z < 0 || x * x + z * z >= radius * radius)) {
            float t = (-dot - sqrtf(disc)) / step2;
            if (t >= 0 && t < 1) {
                *dx = start_x + step_x * t - a->x;
                *dz = start_z + step_z * t - a->z;
                return 1;
            }
        }
    }
    float distance = sqrtf(x * x + z * z);
    if (distance >= radius - .15f) return 0;
    float nx = distance > .01f ? x / distance : host ? 1 : -1;
    float nz = distance > .01f ? z / distance : 0;
    float own_drive = fmaxf(0, -(a->vx * nx + a->vz * nz));
    float peer_drive = fmaxf(0, b->vx * nx + b->vz * nz);
    float share = b->contact & LAN_PLAYER_BLOCKED ? 1 : clamp((1 + peer_drive) / (2 + own_drive + peer_drive), .15f, .85f);
    float correction = fminf((radius - distance) * share, 64);
    *dx = nx * correction; *dz = nz * correction;
    return 1;
}

#ifdef __ANDROID__
#include "sm64.h"
#include "game/mario.h"
#include "game/area.h"
#include "game/camera.h"
#include "game/object_list_processor.h"
#include "engine/surface_collision.h"
#include "engine/math_util.h"
static int blocked;
unsigned char lan_netplay_contact_flags(struct MarioState *m) {
    if (!m || !m->marioObj || m->health < 0x100 || (m->action & ACT_FLAG_INTANGIBLE)
        || (m->action & ACT_GROUP_MASK) == ACT_GROUP_CUTSCENE
        || (m->action & ACT_GROUP_MASK) == ACT_GROUP_AUTOMATIC
        || (m->flags & MARIO_VANISH_CAP) || (gCameraMovementFlags & CAM_MOVE_PAUSE_SCREEN)) return 0;
    return LAN_PLAYER_SOLID | ((m->action & ACT_FLAG_SHORT_HITBOX) ? LAN_PLAYER_SHORT : 0)
         | (blocked ? LAN_PLAYER_BLOCKED : 0);
}
void lan_netplay_apply_contact(struct MarioState *m, const float *start) {
    struct LanPlayerState peer, local = {0};
    float dx, dz;
    blocked = 0;
    if (lan_net_status() != LAN_NET_PAIRED || lan_net_peer_age_ms() > 300 || !lan_net_read_peer(&peer)) return;
    local.x = m->pos[0]; local.y = m->pos[1]; local.z = m->pos[2];
    local.vx = m->vel[0]; local.vz = m->vel[2]; local.contact = lan_netplay_contact_flags(m);
    local.level = gCurrLevelNum; local.area = gCurrAreaIndex;
    if (!lan_contact_solve(&local, &peer, start[0], start[2], lan_net_is_host(), &dx, &dz)) return;
    Vec3f before; vec3f_copy(before, m->pos);
    // Substeps use SM64's actual walls, floor and ceiling on the owning phone.
    int steps = (int)(sqrtf(dx * dx + dz * dz) / 8) + 1;
    if (steps > 24) steps = 24;
    for (int i = 0; i < steps; ++i) {
        Vec3f next = { m->pos[0] + dx / steps, m->pos[1], m->pos[2] + dz / steps };
        struct Surface *floor, *ceil;
        f32_find_wall_collision(&next[0], &next[1], &next[2], 30, 24);
        f32_find_wall_collision(&next[0], &next[1], &next[2], 60, 50);
        float floor_y = find_floor(next[0], next[1] + 50, next[2], &floor);
        float ceil_y = find_ceil(next[0], next[1] + 20, next[2], &ceil);
        float height = local.contact & LAN_PLAYER_SHORT ? 100 : 160;
        if (!floor || floor_y > m->pos[1] + 50 || ceil_y - fmaxf(next[1], floor_y) < height) break;
        if (!(m->action & (ACT_FLAG_AIR | ACT_FLAG_SWIMMING)) && floor_y > next[1]) next[1] = floor_y;
        vec3f_copy(m->pos, next);
        m->floor = floor; m->floorHeight = floor_y;
    }
    float moved_x = m->pos[0] - before[0], moved_z = m->pos[2] - before[2];
    blocked = moved_x * moved_x + moved_z * moved_z < (dx * dx + dz * dz) * .25f;
    for (int i = 0; i < 3; ++i) m->marioObj->header.gfx.pos[i] += m->pos[i] - before[i];
    m->marioObj->oPosX = m->pos[0]; m->marioObj->oPosY = m->pos[1]; m->marioObj->oPosZ = m->pos[2];
}
#endif
