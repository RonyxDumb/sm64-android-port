/* Exercise the real owner-side integration against a small controlled world. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/pc/netplay_contact.c"
struct MarioState gMarioStates[1];
struct MarioState *gMarioState = gMarioStates;
s16 gCurrLevelNum = 9, gCurrAreaIndex = 1, gCameraMovementFlags;
static struct Object object;
static struct Surface ground;
static int wall, no_floor, low_ceil;
static uint32_t age;
static struct LanPlayerState peer;
int lan_net_status(void) { return LAN_NET_PAIRED; }
int lan_net_is_host(void) { return 1; }
uint32_t lan_net_peer_age_ms(void) { return age; }
int lan_net_read_peer(struct LanPlayerState *p) { *p = peer; return 1; }
void *vec3f_copy(Vec3f dst, Vec3f src) { memcpy(dst, src, sizeof(Vec3f)); return dst; }
s32 f32_find_wall_collision(f32 *x, f32 *y, f32 *z, f32 offset, f32 radius) {
    (void)y; (void)z; (void)offset; (void)radius;
    if (wall && *x < 0) *x = 0;
    return 0;
}
f32 find_floor(f32 x, f32 y, f32 z, struct Surface **floor) {
    (void)x; (void)y; (void)z; *floor = no_floor ? NULL : &ground; return 0;
}
f32 find_ceil(f32 x, f32 y, f32 z, struct Surface **ceil) {
    (void)x; (void)y; (void)z; *ceil = NULL; return low_ceil ? 100 : 20000;
}
static void reset(void) {
    memset(gMarioState, 0, sizeof(*gMarioState)); memset(&object, 0, sizeof(object));
    gMarioState->marioObj = &object; gMarioState->health = 0x880;
    gMarioState->action = ACT_IDLE; gMarioState->floor = &ground;
    peer = (struct LanPlayerState){ .x = 60, .level = 9, .area = 1, .act = 6, .contact = LAN_PLAYER_SOLID };
    age = wall = no_floor = low_ceil = gCameraMovementFlags = 0;
}
int main(void) {
    Vec3f start = {0};
    reset(); lan_netplay_apply_contact(gMarioState, start);
    assert(gMarioState->pos[0] < 0 && object.oPosX == gMarioState->pos[0]);
    assert(object.header.gfx.pos[0] == gMarioState->pos[0] && gMarioState->floor == &ground);
    reset(); wall = 1; lan_netplay_apply_contact(gMarioState, start);
    assert(gMarioState->pos[0] == 0 && (lan_netplay_contact_flags(gMarioState) & LAN_PLAYER_BLOCKED));
    reset(); no_floor = 1; lan_netplay_apply_contact(gMarioState, start); assert(gMarioState->pos[0] == 0);
    reset(); low_ceil = 1; lan_netplay_apply_contact(gMarioState, start); assert(gMarioState->pos[0] == 0);
    reset(); age = 301; lan_netplay_apply_contact(gMarioState, start); assert(gMarioState->pos[0] == 0);
    reset(); gCameraMovementFlags = CAM_MOVE_PAUSE_SCREEN; lan_netplay_apply_contact(gMarioState, start);
    assert(gMarioState->pos[0] == 0 && !lan_netplay_contact_flags(gMarioState));
    reset(); gMarioState->action = ACT_STAR_DANCE_EXIT;
    assert(!lan_netplay_contact_flags(gMarioState));
    puts("world contact: synchronized mesh, walls, floor/ceiling, stale packets, pause and cutscene PASS");
}
