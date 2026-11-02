#ifndef NETPLAY_DIRECT_H
#define NETPLAY_DIRECT_H
int net_direct_start(int host);
void net_direct_stop(void);
int net_direct_status(void);
const char *net_direct_host_address(void);
const char *net_direct_detail(void);
const char *net_direct_games(void);
#endif
