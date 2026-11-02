/* Two independent processes, real nonblocking UDP, mocked Android group setup. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "netplay_lan.h"
int net_direct_start(int host) { (void)host; return 0; }
void net_direct_stop(void) {}
int net_direct_status(void) { return 1; }
const char *net_direct_host_address(void) { return "127.0.0.1"; }
const char *net_direct_detail(void) { return "TEST DIRECT GROUP"; }
int main(int argc, char **argv) {
    int host = argc > 1 && !strcmp(argv[1], "host");
    int ticks, got_state = 0, got_ready = 0, got_progress = 0, got_warp = host;
    struct LanPlayerState local = {0}, peer;
    struct LanWarpState warp;
    uint8_t stars[LAN_PROGRESS_COURSES];
    uint32_t keys;
    setenv("NETPLAY_TEST_BIND_IP", host ? "127.0.0.1" : "127.0.0.2", 1);
    assert(lan_net_start(host, LAN_TRANSPORT_DIRECT) == 0);
    local.x = host ? 123.5f : -42.25f;
    local.y = 200; local.z = -300; local.level = 9; local.area = 1; local.act = host ? 1 : 6;
    local.vx = host ? 12 : -8; local.vz = 4; local.contact = 1;
    local.anim = host ? 72 : 15; local.frame = 17; local.anim_y = 189; local.anim_accel = 65536;
    for (ticks = 0; ticks < 240; ++ticks) {
        lan_net_tick();
        if (lan_net_status() == LAN_NET_PAIRED) {
            lan_net_set_ready();
            lan_net_send_player(&local);
            lan_net_collect(host ? 1 : 2, host ? 4 : 8, host ? 1 : 2);
            if (host && !got_state) lan_net_publish_warp(9, 1, 10, 0, 2);
        }
        got_ready |= lan_net_both_ready();
        if (lan_net_read_peer(&peer)) {
            assert(peer.x == (host ? -42.25f : 123.5f));
            assert(peer.anim == (host ? 15 : 72));
            assert(peer.frame == 17 && peer.anim_y == 189 && peer.anim_accel == 65536);
            assert(peer.act == (host ? 6 : 1));
            assert(peer.vx == (host ? -8 : 12) && peer.vz == 4 && peer.contact == 1);
            got_state = 1;
        }
        lan_net_progress(stars, &keys);
        got_progress |= stars[1] == 4 && stars[2] == 8 && keys == 3;
        if (lan_net_read_warp(&warp)) {
            assert(warp.level == 9 && warp.area == 1 && warp.node == 10 && warp.act == 2);
            got_warp = 1;
        }
        if (ticks > 80 && got_state && got_ready && got_progress && got_warp) break;
        usleep(10000);
    }
    assert(got_state && got_ready && got_progress && got_warp);
    lan_net_stop();
    assert(lan_net_status() == LAN_NET_OFF && !lan_net_both_ready());
    lan_net_progress(stars, &keys);
    assert(keys == 0 && stars[1] == 0 && stars[2] == 0);
    printf("%s: independent animation, ready handshake, warp, progress and reset PASS\n", host ? "host" : "guest");
    return 0;
}
