/* Exercise malformed network input, replay/loss behavior and timeout. */
#include <assert.h>
#include <stdio.h>
#include "../src/pc/netplay_lan.c"
int net_direct_start(int host) { (void)host; return 0; }
void net_direct_stop(void) {}
int net_direct_status(void) { return 1; }
const char *net_direct_host_address(void) { return "127.0.0.1"; }
const char *net_direct_detail(void) { return "TEST"; }
static void inject(int peer, uint8_t *packet, size_t size) {
    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET; dest.sin_port = htons(LAN_PORT);
    inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);
    assert(sendto(peer, packet, size, 0, (struct sockaddr *)&dest, sizeof(dest)) == (ssize_t)size);
    lan_net_tick();
}
int main(void) {
    uint8_t packet[LAN_PACKET_SIZE + 1] = {0};
    int peer;
    struct sockaddr_in addr = {0};
    struct LanPlayerState state;
    setenv("NETPLAY_TEST_BIND_IP", "127.0.0.1", 1);
    assert(lan_net_start_host() == 0);
    peer = socket(AF_INET, SOCK_DGRAM, 0);
    assert(peer >= 0);
    addr.sin_family = AF_INET; addr.sin_port = htons(LAN_PORT);
    inet_pton(AF_INET, "127.0.0.2", &addr.sin_addr);
    assert(bind(peer, (struct sockaddr *)&addr, sizeof(addr)) == 0);
    memcpy(packet, "SM64", 4); packet[4] = LAN_PROTOCOL_VERSION; packet[5] = LAN_HELLO;
    write32(packet + 8, sOwnNonce ^ 1234);
    packet[4] = 1; inject(peer, packet, LAN_PACKET_SIZE);
    assert(lan_net_status() == LAN_NET_SEARCHING);
    packet[4] = LAN_PROTOCOL_VERSION;
    inject(peer, packet, LAN_PACKET_SIZE + 1);
    assert(lan_net_status() == LAN_NET_SEARCHING);
    inject(peer, packet, LAN_PACKET_SIZE);
    assert(lan_net_status() == LAN_NET_PAIRED);
    packet[5] = LAN_PLAYER_STATE; packet[16] = 9; packet[17] = 1;
    write32(packet + 12, 1); write_float(packet + 20, 5.5f);
    write16(packet + 40, 72); write16(packet + 42, 2);
    write32(packet + 36, sOwnNonce ^ 1);
    inject(peer, packet, LAN_PACKET_SIZE);
    assert(!lan_net_read_peer(&state)); // wrong session token
    write32(packet + 36, sOwnNonce);
    inject(peer, packet, LAN_PACKET_SIZE);
    assert(lan_net_read_peer(&state) && state.x == 5.5f);
    write32(packet + 12, 2); write_float(packet + 20, NAN);
    inject(peer, packet, LAN_PACKET_SIZE);
    assert(lan_net_read_peer(&state) && state.x == 5.5f);
    assert(sLastStateSeq == 1); // malformed high sequence cannot poison valid state
    write_float(packet + 20, 6.5f);
    inject(peer, packet, LAN_PACKET_SIZE);
    assert(lan_net_read_peer(&state) && state.x == 6.5f);
    write32(packet + 12, 1); write_float(packet + 20, 100);
    inject(peer, packet, LAN_PACKET_SIZE);
    assert(lan_net_read_peer(&state) && state.x == 6.5f); // stale packet
    lan_net_set_ready();
    lan_net_collect(1, 4, 1);
    sLastPeerTick = now_ms() - LAN_RECONNECT_AFTER_MS - 1;
    lan_net_tick();
    assert(lan_net_status() == LAN_NET_RECONNECTING);
    packet[5] = LAN_KEEPALIVE;
    inject(peer, packet, LAN_PACKET_SIZE);
    assert(lan_net_status() == LAN_NET_PAIRED && lan_net_ready_mask() == 1);
    assert(sStars[1] == 4 && sKeys == 1); // reconnect preserves earned progress
    sLastPeerTick = now_ms() - LAN_TIMEOUT_MS - 1;
    lan_net_tick();
    assert(lan_net_status() == LAN_NET_ERROR && !lan_net_read_peer(&state));
    close(peer); lan_net_stop();
    puts("packets: version, oversize, wrong session, NaN, replay, timeout PASS");
    return 0;
}
