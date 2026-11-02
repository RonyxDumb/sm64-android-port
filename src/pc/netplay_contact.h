#ifndef NETPLAY_CONTACT_H
#define NETPLAY_CONTACT_H
#include "netplay_lan.h"
#define LAN_PLAYER_SOLID 1
#define LAN_PLAYER_SHORT 2
#define LAN_PLAYER_BLOCKED 4
/* Corrections are applied only to the locally owned player. */
int lan_contact_solve(const struct LanPlayerState *local, const struct LanPlayerState *peer,
                      float start_x, float start_z, int host, float *dx, float *dz);
#ifdef __ANDROID__
struct MarioState;
unsigned char lan_netplay_contact_flags(struct MarioState *m);
void lan_netplay_apply_contact(struct MarioState *m, const float *start);
#endif
#endif
