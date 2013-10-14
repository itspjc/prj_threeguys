#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <stdio.h>

#include "eventloop.h"
#include "socket.h"

#define MAXSESSIONS 5


void init_player();
void udp_data_recv(RTSP_SOCK fd);
void handle_command(const char *c);


#endif
