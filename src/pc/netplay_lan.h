#ifndef SM64_ANDROID_NETPLAY_LAN_H
#define SM64_ANDROID_NETPLAY_LAN_H
#include <stdint.h>
enum LanNetStatus { LAN_NET_OFF, LAN_NET_SEARCHING, LAN_NET_PAIRED, LAN_NET_ERROR, LAN_NET_RECONNECTING };
enum LanTransport { LAN_TRANSPORT_WIFI, LAN_TRANSPORT_DIRECT };
struct LanPlayerState {
    float x, y, z;
    float vx, vz;
    uint8_t contact;
    int16_t yaw, pitch, roll;
    uint8_t level, area, act;
    uint32_t action;
    int16_t anim, frame, anim_y;
    int32_t anim_accel;
};
#define LAN_PROGRESS_COURSES 26 /* slot 0 secret stars, slot n course n */
struct LanWarpState { uint32_t serial, arg; uint8_t level, area, node, act; };
void lan_net_publish_warp(uint8_t level, uint8_t area, uint8_t node, uint32_t arg, uint8_t act);
int lan_net_read_warp(struct LanWarpState *out);
void lan_net_collect(uint8_t course, uint8_t stars, uint32_t keys);
void lan_net_progress(uint8_t *stars, uint32_t *keys);
int lan_net_start(int host, int transport);
int lan_net_start_host(void);
int lan_net_start_join(void);
void lan_net_stop(void);
void lan_net_tick(void);
int lan_net_status(void);
int lan_net_is_host(void);
void lan_net_set_ready(void);
int lan_net_ready_mask(void);
int lan_net_both_ready(void);
const char *lan_net_detail(void);
void lan_net_send_player(const struct LanPlayerState *state);
uint32_t lan_net_peer_age_ms(void);
int lan_net_read_peer(struct LanPlayerState *out);
#endif
