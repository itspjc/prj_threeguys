
#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_

#include "socket.h"

extern u_long inow;            /* Internal time format (ms) */ 

extern RTSP_SOCK main_fd;
extern RTSP_SOCK main_fd_read;
void (*readers[MAX_FDS])(RTSP_SOCK);

void eventloop_init();
void eventloop();

#endif /* _EVENTLOOP_H_ */
