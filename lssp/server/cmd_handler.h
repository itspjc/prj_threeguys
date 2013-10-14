
#ifndef _CMD_HANDLER_
#define _CMD_HANDLER_

#include "socket.h"

void handle_command(const char *c);
void udp_data_recv(RTSP_SOCK fd);

#endif	/* _CMD_HANDLER_ */
