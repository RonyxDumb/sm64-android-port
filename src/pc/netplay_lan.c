/*
 * SM64 Android local-network transport.
 *
 * 2-player offline LAN / phone-hotspot prototype, NOT Nintendo DS raw Wi-Fi.
 * Discovery: guest broadcasts HELLO on UDP/45565; host returns WELCOME.
 * Transport: versioned, fixed-size network-order packets, nonblocking I/O.
 * Wi-Fi Direct pairing is confirmed by Android; LAN accepts the first peer.
 * Version 2 includes recipient nonce, readiness and independent animation.
 */
#include "netplay_lan.h"
#include <string.h>

#if defined(__ANDROID__) || defined(NETPLAY_SOCKET_TEST)

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include "netplay_direct.h"
#include <fcntl.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>

#define LAN_PORT 45565
#define LAN_PACKET_SIZE 112
#define LAN_PROTOCOL_VERSION 2
#define LAN_DISCOVER_INTERVAL 1000
#define LAN_KEEPALIVE_INTERVAL 500
#define LAN_TIMEOUT_FRAMES 10000
#define LAN_PEER_STATE_MAX_AGE 3000
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
static int sTransport;
static int sOwnReady, sPeerReady;
static uint32_t sLastDiscover, sLastKeepalive;
static uint32_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
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
static struct LanWarpState sWarp;
static uint8_t sStars[LAN_PROGRESS_COURSES];
static uint32_t sKeys;

void lan_net_publish_warp(uint8_t level, uint8_t area, uint8_t node, uint32_t arg, uint8_t act) {
    if (!sHost || sStatus != LAN_NET_PAIRED || node >= 0xF8) return;
    ++sWarp.serial;
    sWarp.level = level; sWarp.area = area; sWarp.node = node; sWarp.arg = arg; sWarp.act = act;
}
int lan_net_read_warp(struct LanWarpState *out) {
    if (!out || sHost || sStatus != LAN_NET_PAIRED || !sWarp.serial) return 0;
    *out = sWarp; return 1;
}
void lan_net_collect(uint8_t course, uint8_t stars, uint32_t keys) {
    if (sStatus != LAN_NET_PAIRED) return;
    if (course < LAN_PROGRESS_COURSES) sStars[course] |= stars & 0x7F;
    sKeys |= keys & 3;
}
void lan_net_progress(uint8_t *stars, uint32_t *keys) {
    memcpy(stars, sStars, sizeof(sStars)); *keys = sKeys;
}

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
    write32(buf + 36, sPeerNonce); /* bind every post-discovery packet to this session */
    buf[6] = sOwnReady != 0;
    if (state != NULL) {
        buf[16] = state->level;
        buf[17] = state->area;
        write16(buf + 18, (uint16_t)state->yaw);
        write_float(buf + 20, state->x);
        write_float(buf + 24, state->y);
        write_float(buf + 28, state->z);
        write32(buf + 32, state->action);
        write16(buf + 40, (uint16_t)state->anim);
        write16(buf + 42, (uint16_t)state->frame);
        write16(buf + 44, (uint16_t)state->anim_y);
        write16(buf + 46, (uint16_t)state->pitch);
        write16(buf + 48, (uint16_t)state->roll);
        buf[50] = state->act;
        write32(buf + 52, (uint32_t)state->anim_accel);
    }
    if (type == LAN_KEEPALIVE || type == LAN_PLAYER_STATE) {
        write32(buf + 56, sHost ? sWarp.serial : 0);
        buf[60] = sWarp.level; buf[61] = sWarp.area; buf[62] = sWarp.node; buf[63] = sWarp.act;
        write32(buf + 64, sWarp.arg);
        memcpy(buf + 68, sStars, sizeof(sStars));
        write32(buf + 96, sKeys);
    }
    (void)sendto(sSocket, buf, sizeof(buf), 0,
                 (const struct sockaddr *)addr, sizeof(*addr));
}

void lan_net_stop(void) {
    if (sTransport == LAN_TRANSPORT_DIRECT) net_direct_stop();
    sTransport = LAN_TRANSPORT_WIFI;
    sOwnReady = sPeerReady = 0;
    sLastDiscover = sLastKeepalive = 0;
    if (sSocket >= 0) close(sSocket);
    sSocket = -1;
    sStatus = LAN_NET_OFF;
    sHost = 0;
    sOwnNonce = 0;
    sPeerNonce = 0;
    sSeq = sTick = 0;
    sLastPeerTick = sLastStateTick = sLastStateSeq = 0;
    sHavePeerState = 0;
    memset(&sWarp, 0, sizeof(sWarp));
    memset(sStars, 0, sizeof(sStars));
    sKeys = 0;
    memset(&sPeerAddress, 0, sizeof(sPeerAddress));
    memset(&sPeerState, 0, sizeof(sPeerState));
}

int lan_net_start(int host, int transport) {
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
#ifdef NETPLAY_SOCKET_TEST
    /* Loopback aliases let two production transports run on one test machine. */
    {
        const char *test_address = getenv("NETPLAY_TEST_BIND_IP");
        if (test_address && inet_pton(AF_INET, test_address, &bind_address.sin_addr) != 1) goto fail;
    }
#endif
    if (bind(sSocket, (struct sockaddr *)&bind_address, sizeof(bind_address)) < 0) goto fail;

    sOwnNonce = (uint32_t)time(NULL) ^ ((uint32_t)getpid() << 16) ^ (uint32_t)(uintptr_t)&sSocket;
    if (sOwnNonce == 0) sOwnNonce = 1;
    sHost = host;
    sTransport = transport;
    if (transport == LAN_TRANSPORT_DIRECT && net_direct_start(host) < 0) goto fail;
    sStatus = LAN_NET_SEARCHING;
    return 0;
fail:
    lan_net_stop();
    sStatus = LAN_NET_ERROR;
    return -1;
}

int lan_net_start_host(void) { return lan_net_start(1, LAN_TRANSPORT_WIFI); }
int lan_net_start_join(void) { return lan_net_start(0, LAN_TRANSPORT_WIFI); }
int lan_net_status(void) { return sStatus; }
int lan_net_is_host(void) { return sHost; }
void lan_net_set_ready(void) { sOwnReady = 1; sLastKeepalive = 0; }
int lan_net_both_ready(void) { return sStatus == LAN_NET_PAIRED && sOwnReady && sPeerReady; }
const char *lan_net_detail(void) {
    return sTransport == LAN_TRANSPORT_DIRECT ? net_direct_detail() : "SAME WIFI OR HOTSPOT";
}

void lan_net_tick(void) {
    uint8_t buf[LAN_PACKET_SIZE + 1];
    struct sockaddr_in src;
    socklen_t src_len;
    ssize_t n;
    int count;

    if (sStatus != LAN_NET_SEARCHING && sStatus != LAN_NET_PAIRED) return;
    sTick = now_ms();
    if (sTransport == LAN_TRANSPORT_DIRECT) {
        int direct = net_direct_status();
        if (direct < 0) { sStatus = LAN_NET_ERROR; return; }
        if (!direct) return;
    }

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
        if (!sHost && sStatus == LAN_NET_SEARCHING && buf[5] == LAN_WELCOME && read32(buf + 36) == sOwnNonce) {
            sPeerAddress = src;
            sPeerNonce = read32(buf + 8);
            sStatus = LAN_NET_PAIRED;
            sLastPeerTick = sTick;
            sPeerReady = buf[6] != 0;
            continue;
        }
        if (sStatus != LAN_NET_PAIRED || !same_peer(&src)
            || read32(buf + 8) != sPeerNonce || read32(buf + 36) != sOwnNonce) continue;
        sPeerReady |= buf[6] != 0;
        if (buf[5] == LAN_KEEPALIVE || buf[5] == LAN_PLAYER_STATE) {
            int i;
            sLastPeerTick = sTick;
            /* Monotonic progress and the latest host warp are retransmitted in
             * every heartbeat/state packet. Duplicates and loss are harmless. */
            for (i = 0; i < LAN_PROGRESS_COURSES; ++i) sStars[i] |= buf[68 + i] & 0x7F;
            sKeys |= read32(buf + 96) & 3;
            if (!sHost && read32(buf + 56) && buf[60] >= 4 && buf[60] <= 38
                && buf[61] >= 1 && buf[61] <= 4 && buf[62] < 0xF8
                && (int32_t)(read32(buf + 56) - sWarp.serial) > 0) {
                sWarp.serial = read32(buf + 56);
                sWarp.level = buf[60]; sWarp.area = buf[61]; sWarp.node = buf[62];
                sWarp.act = buf[63]; sWarp.arg = read32(buf + 64);
            }
        }
        if (buf[5] == LAN_PLAYER_STATE) {
            uint32_t seq = read32(buf + 12);
            if (!sHavePeerState || (int32_t)(seq - sLastStateSeq) > 0) {
                struct LanPlayerState previous = sPeerState;
                sPeerState.level = buf[16];
                sPeerState.area = buf[17];
                sPeerState.yaw = (int16_t)read16(buf + 18);
                sPeerState.x = read_float(buf + 20);
                sPeerState.y = read_float(buf + 24);
                sPeerState.z = read_float(buf + 28);
                sPeerState.action = read32(buf + 32);
                sPeerState.anim = (int16_t)read16(buf + 40);
                sPeerState.frame = (int16_t)read16(buf + 42);
                sPeerState.anim_y = (int16_t)read16(buf + 44);
                sPeerState.pitch = (int16_t)read16(buf + 46);
                sPeerState.roll = (int16_t)read16(buf + 48);
                sPeerState.act = buf[50];
                sPeerState.anim_accel = (int32_t)read32(buf + 52);
                if (!isfinite(sPeerState.x) || !isfinite(sPeerState.y) || !isfinite(sPeerState.z)
                    || fabsf(sPeerState.x) > 32768 || fabsf(sPeerState.y) > 32768
                    || fabsf(sPeerState.z) > 32768 || sPeerState.area > 8
                    || sPeerState.anim < 0 || sPeerState.anim > 255
                    || sPeerState.frame < 0 || sPeerState.frame > 10000) {
                    sPeerState = previous;
                    continue;
                }
                sLastStateSeq = seq;
                sLastStateTick = sTick;
                sHavePeerState = 1;
            }
        }
    }

    if (sStatus == LAN_NET_SEARCHING && !sHost && (sLastDiscover == 0 || sTick - sLastDiscover >= LAN_DISCOVER_INTERVAL)) {
        struct sockaddr_in broadcast;
        memset(&broadcast, 0, sizeof(broadcast));
        broadcast.sin_family = AF_INET;
        broadcast.sin_port = htons(LAN_PORT);
        broadcast.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        if (sTransport == LAN_TRANSPORT_DIRECT) {
            const char *ip = net_direct_host_address();
            if (!ip || inet_pton(AF_INET, ip, &broadcast.sin_addr) != 1) return;
        }
        sLastDiscover = sTick;
        send_packet(&broadcast, LAN_HELLO, NULL);
    } else if (sStatus == LAN_NET_PAIRED) {
        if (sTick - sLastPeerTick > LAN_TIMEOUT_FRAMES) {
            sStatus = LAN_NET_ERROR;
            sPeerNonce = 0;
            sHavePeerState = 0;
            sOwnReady = sPeerReady = 0;
            memset(&sPeerAddress, 0, sizeof(sPeerAddress));
        } else if ((sLastKeepalive == 0 || sTick - sLastKeepalive >= LAN_KEEPALIVE_INTERVAL)) {
            send_packet(&sPeerAddress, LAN_KEEPALIVE, NULL);
            sLastKeepalive = sTick;
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
void lan_net_publish_warp(uint8_t level, uint8_t area, uint8_t node, uint32_t arg, uint8_t act) {
    (void)level; (void)area; (void)node; (void)arg; (void)act;
}
int lan_net_read_warp(struct LanWarpState *out) { (void)out; return 0; }
void lan_net_collect(uint8_t course, uint8_t stars, uint32_t keys) { (void)course; (void)stars; (void)keys; }
void lan_net_progress(uint8_t *stars, uint32_t *keys) { memset(stars, 0, LAN_PROGRESS_COURSES); *keys = 0; }
int lan_net_start(int host, int transport) { (void)host; (void)transport; return -1; }
void lan_net_set_ready(void) {}
int lan_net_both_ready(void) { return 0; }
const char *lan_net_detail(void) { return "ANDROID ONLY"; }
int lan_net_start_host(void) { return -1; }
int lan_net_start_join(void) { return -1; }
void lan_net_stop(void) {}
void lan_net_tick(void) {}
int lan_net_status(void) { return LAN_NET_OFF; }
int lan_net_is_host(void) { return 0; }
void lan_net_send_player(const struct LanPlayerState *state) { (void)state; }
int lan_net_read_peer(struct LanPlayerState *out) { (void)out; return 0; }
#endif
