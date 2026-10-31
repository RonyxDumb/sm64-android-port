#ifndef SM64_ANDROID_NETPLAY_LAN_H
#define SM64_ANDROID_NETPLAY_LAN_H

#include <stdint.h>

/* LAN transport for the Android cooperative prototype. This does not
 * implement game-world authority or object replication. */
enum LanNetStatus {
    LAN_NET_OFF = 0,
    LAN_NET_SEARCHING = 1,
    LAN_NET_PAIRED = 2,
    LAN_NET_ERROR = 3,
};

struct LanPlayerState {
    float x, y, z;
    int16_t yaw;
    uint8_t level;
    uint8_t area;
    uint32_t action;
};

int lan_net_start_host(void);
int lan_net_start_join(void);
void lan_net_stop(void);
void lan_net_tick(void);
int lan_net_status(void);
int lan_net_is_host(void);
void lan_net_send_player(const struct LanPlayerState *state);
int lan_net_read_peer(struct LanPlayerState *out);

#endif
