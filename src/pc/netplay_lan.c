/*
 * SM64 Android local-network transport.
 *
 * 2-player offline LAN / phone-hotspot prototype, NOT Nintendo DS raw Wi-Fi.
 * Discovery: guest broadcasts HELLO on UDP/45565; host returns WELCOME.
 * Transport: versioned, fixed-size network-order packets, nonblocking I/O.
 * The host accepts the first nearby peer automatically. Do not expose this
 * UDP port to the Internet; explicit approval/pairing is future work.
 */
#include "netplay_lan.h"
#include <string.h>

#if defined(__ANDROID__)

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#define LAN_PORT 45565
#define LAN_PACKET_SIZE 40
#define LAN_PROTOCOL_VERSION 1
#define LAN_DISCOVER_INTERVAL 30
#define LAN_KEEPALIVE_INTERVAL 30
#define LAN_TIMEOUT_FRAMES 180
#define LAN_PEER_STATE_MAX_AGE 120
#define LAN_RX_MAX_PER_TICK 24

enum LanPacketType {
    LAN_HELLO = 1,
    LAN_WELCOME = 2,
    LAN_PLAYER_STATE = 3,
    LAN_KEEPALIVE = 4,
};

static int sSocket = -1;
static int sStatus = LAN_NET_OFF;
static int sHost;
static struct sockaddr_in sPeerAddress;
static uint32_t sOwnNonce;
static uint32_t sPeerNonce;
static uint32_t sSeq;
static uint32_t sTick;
static uint32_t sLastPeerTick;
static uint32_t sLastStateTick;
static uint32_t sLastStateSeq;
static int sHavePeerState;
static struct LanPlayerState sPeerState;

static void write32(uint8_t *dest, uint32_t value) {
    uint32_t network = htonl(value);
    memcpy(dest, &network, sizeof(network));
}

static uint32_t read32(const uint8_t *src) {
    uint32_t network;
    memcpy(&network, src, sizeof(network));
    return ntohl(network);
}

static void write16(uint8_t *dest, uint16_t value) {
    uint16_t network = htons(value);
    memcpy(dest, &network, sizeof(network));
}

static uint16_t read16(const uint8_t *src) {
    uint16_t network;
    memcpy(&network, src, sizeof(network));
    return ntohs(network);
}

static void write_float(uint8_t *dest, float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    write32(dest, bits);
}

static float read_float(const uint8_t *src) {
    uint32_t bits = read32(src);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int same_peer(const struct sockaddr_in *addr) {
    return addr->sin_family == AF_INET
        && addr->sin_port == sPeerAddress.sin_port
        && addr->sin_addr.s_addr == sPeerAddress.sin_addr.s_addr;
}

static void send_packet(const struct sockaddr_in *addr, uint8_t type,
                        const struct LanPlayerState *state) {
    uint8_t buf[LAN_PACKET_SIZE] = {0};
    if (sSocket < 0) return;
    memcpy(buf, "SM64", 4);
    buf[4] = LAN_PROTOCOL_VERSION;
    buf[5] = type;
    write32(buf + 8, sOwnNonce);
    write32(buf + 12, ++sSeq);
    if (state != NULL) {
        buf[16] = state->level;
        buf[17] = state->area;
        write16(buf + 18, (uint16_t)state->yaw);
        write_float(buf + 20, state->x);
        write_float(buf + 24, state->y);
        write_float(buf + 28, state->z);
        write32(buf + 32, state->action);
    }
    (void)sendto(sSocket, buf, sizeof(buf), 0,
                 (const struct sockaddr *)addr, sizeof(*addr));
}

void lan_net_stop(void) {
    if (sSocket >= 0) close(sSocket);
    sSocket = -1;
    sStatus = LAN_NET_OFF;
    sHost = 0;
    sOwnNonce = 0;
    sPeerNonce = 0;
    sSeq = sTick = 0;
    sLastPeerTick = sLastStateTick = sLastStateSeq = 0;
    sHavePeerState = 0;
    memset(&sPeerAddress, 0, sizeof(sPeerAddress));
    memset(&sPeerState, 0, sizeof(sPeerState));
}

static int lan_start(int host) {
    struct sockaddr_in bind_address;
    int yes = 1;
    int flags;
    lan_net_stop();

    sSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sSocket < 0) goto fail;
    if (setsockopt(sSocket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0) goto fail;
    flags = fcntl(sSocket, F_GETFL, 0);
    if (flags < 0 || fcntl(sSocket, F_SETFL, flags | O_NONBLOCK) < 0) goto fail;
    memset(&bind_address, 0, sizeof(bind_address));
    bind_address.sin_family = AF_INET;
    bind_address.sin_port = htons(LAN_PORT);
    bind_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sSocket, (struct sockaddr *)&bind_address, sizeof(bind_address)) < 0) goto fail;

    sOwnNonce = (uint32_t)time(NULL) ^ ((uint32_t)getpid() << 16) ^ (uint32_t)(uintptr_t)&sSocket;
    if (sOwnNonce == 0) sOwnNonce = 1;
    sHost = host;
    sStatus = LAN_NET_SEARCHING;
    return 0;
fail:
    lan_net_stop();
    sStatus = LAN_NET_ERROR;
    return -1;
}

int lan_net_start_host(void) { return lan_start(1); }
int lan_net_start_join(void) { return lan_start(0); }
int lan_net_status(void) { return sStatus; }
int lan_net_is_host(void) { return sHost; }

void lan_net_tick(void) {
    uint8_t buf[LAN_PACKET_SIZE];
    struct sockaddr_in src;
    socklen_t src_len;
    ssize_t n;
    int count;

    if (sStatus != LAN_NET_SEARCHING && sStatus != LAN_NET_PAIRED) return;
    ++sTick;

    for (count = 0; count < LAN_RX_MAX_PER_TICK; ++count) {
        src_len = sizeof(src);
        n = recvfrom(sSocket, buf, sizeof(buf), 0, (struct sockaddr *)&src, &src_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) break;
            sStatus = LAN_NET_ERROR;
            return;
        }
        if (n != LAN_PACKET_SIZE || memcmp(buf, "SM64", 4) != 0
            || buf[4] != LAN_PROTOCOL_VERSION || read32(buf + 8) == sOwnNonce) continue;

        if (sHost && buf[5] == LAN_HELLO) {
            if (sStatus == LAN_NET_SEARCHING) {
                sPeerAddress = src;
                sPeerNonce = read32(buf + 8);
                sStatus = LAN_NET_PAIRED;
                sLastPeerTick = sTick;
            }
            if (same_peer(&src) && read32(buf + 8) == sPeerNonce) {
                sLastPeerTick = sTick;
                send_packet(&sPeerAddress, LAN_WELCOME, NULL);
            }
            continue;
        }
        if (!sHost && sStatus == LAN_NET_SEARCHING && buf[5] == LAN_WELCOME) {
            sPeerAddress = src;
            sPeerNonce = read32(buf + 8);
            sStatus = LAN_NET_PAIRED;
            sLastPeerTick = sTick;
            continue;
        }
        if (sStatus != LAN_NET_PAIRED || !same_peer(&src)
            || read32(buf + 8) != sPeerNonce) continue;
        if (buf[5] == LAN_KEEPALIVE || buf[5] == LAN_PLAYER_STATE) {
            sLastPeerTick = sTick;
        }
        if (buf[5] == LAN_PLAYER_STATE) {
            uint32_t seq = read32(buf + 12);
            if (!sHavePeerState || (int32_t)(seq - sLastStateSeq) > 0) {
                sLastStateSeq = seq;
                sPeerState.level = buf[16];
                sPeerState.area = buf[17];
                sPeerState.yaw = (int16_t)read16(buf + 18);
                sPeerState.x = read_float(buf + 20);
                sPeerState.y = read_float(buf + 24);
                sPeerState.z = read_float(buf + 28);
                sPeerState.action = read32(buf + 32);
                sLastStateTick = sTick;
                sHavePeerState = 1;
            }
        }
    }

    if (sStatus == LAN_NET_SEARCHING && !sHost && sTick % LAN_DISCOVER_INTERVAL == 1) {
        struct sockaddr_in broadcast;
        memset(&broadcast, 0, sizeof(broadcast));
        broadcast.sin_family = AF_INET;
        broadcast.sin_port = htons(LAN_PORT);
        broadcast.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        send_packet(&broadcast, LAN_HELLO, NULL);
    } else if (sStatus == LAN_NET_PAIRED) {
        if (sTick - sLastPeerTick > LAN_TIMEOUT_FRAMES) {
            sStatus = LAN_NET_SEARCHING;
            sPeerNonce = 0;
            sHavePeerState = 0;
            memset(&sPeerAddress, 0, sizeof(sPeerAddress));
        } else if (sTick % LAN_KEEPALIVE_INTERVAL == 0) {
            send_packet(&sPeerAddress, LAN_KEEPALIVE, NULL);
        }
    }
}

void lan_net_send_player(const struct LanPlayerState *state) {
    if (state != NULL && sStatus == LAN_NET_PAIRED) {
        send_packet(&sPeerAddress, LAN_PLAYER_STATE, state);
    }
}

int lan_net_read_peer(struct LanPlayerState *out) {
    if (out == NULL || sStatus != LAN_NET_PAIRED || !sHavePeerState
        || sTick - sLastStateTick > LAN_PEER_STATE_MAX_AGE) return 0;
    *out = sPeerState;
    return 1;
}

#else
/* Non-Android builds preserve the original single-player behavior. */
int lan_net_start_host(void) { return -1; }
int lan_net_start_join(void) { return -1; }
void lan_net_stop(void) {}
void lan_net_tick(void) {}
int lan_net_status(void) { return LAN_NET_OFF; }
int lan_net_is_host(void) { return 0; }
void lan_net_send_player(const struct LanPlayerState *state) { (void)state; }
int lan_net_read_peer(struct LanPlayerState *out) { (void)out; return 0; }
#endif
