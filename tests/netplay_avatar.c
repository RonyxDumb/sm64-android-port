/* Exercise the actual avatar spawn/delete path across missions and areas. */
#include <assert.h>
#include <stdio.h>
#include "../src/pc/netplay_game.c"
struct MarioState gMarioStates[1];
struct MarioState *gMarioState = gMarioStates;
struct Object *gMarioObject;
static struct GraphNode *nodes[256];
struct GraphNode **gLoadedGraphNodes = nodes;
s16 gCurrLevelNum;
s16 gCurrSaveFileNum;
s16 gCurrActNum;
s16 gCurrAreaIndex;
const BehaviorScript bhvLanRemoteMario[] = {0};
static struct Object local, remote;
static struct LanPlayerState state;
static int spawned, deleted, connected = 1;
int lan_net_status(void) { return connected ? LAN_NET_PAIRED : LAN_NET_ERROR; }
int lan_net_is_host(void) { return 1; }
int lan_net_read_peer(struct LanPlayerState *out) { *out = state; return connected; }
void lan_net_progress(uint8_t *stars, uint32_t *keys) { memset(stars, 0, LAN_PROGRESS_COURSES); *keys = 0; }
u32 save_file_get_star_flags(s32 file, s32 course) { (void)file; (void)course; return 0; }
void save_file_set_star_flags(s32 file, s32 course, u32 flags) { (void)file; (void)course; (void)flags; }
u32 save_file_get_flags(void) { return 0; }
void save_file_set_flags(u32 flags) { (void)flags; }
s32 save_file_get_total_star_count(s32 file, s32 min, s32 max) { (void)file; (void)min; (void)max; return 0; }
struct Object *spawn_object(struct Object *parent, s32 model, const BehaviorScript *behavior) {
    (void)parent; (void)model; ++spawned;
    memset(&remote, 0, sizeof(remote)); remote.activeFlags = ACTIVE_FLAG_ACTIVE;
    remote.behavior = behavior; return &remote;
}
void obj_mark_for_deletion(struct Object *obj) { ++deleted; obj->activeFlags = 0; }
int main(void) {
    gMarioObject = &local; gCurrLevelNum = LEVEL_BOB; gCurrAreaIndex = 1; gCurrActNum = 1;
    state = (struct LanPlayerState){ .level = LEVEL_BOB, .area = 1, .act = 6, .x = 100, .y = 200 };
    lan_netplay_object_tick();
    assert(spawned == 1 && sRemoteMario == &remote && remote.oPosX == 100);
    state.act = 3; lan_netplay_object_tick(); assert(spawned == 1 && deleted == 0);
    state.area = 2; lan_netplay_object_tick(); assert(deleted == 1 && !sRemoteMario);
    state.area = 1; lan_netplay_object_tick(); assert(spawned == 2 && sRemoteMario);
    connected = 0; lan_netplay_object_tick(); assert(deleted == 2 && !sRemoteMario);
    puts("avatar: different missions visible; area exit, reunion and disconnect PASS");
}
