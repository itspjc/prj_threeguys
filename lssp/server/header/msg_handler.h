
#ifndef _MSG_HANDLER_H_
#define _MSG_HANDLER_H_

#define BLEN            4096
#define STATUS_MSG_LEN  256

void msg_handler();



#include "header/socket.h"
#include "header/session.h"

u_short RTSP_last_request;       /* code for last request message sent */
u_short RTSP_send_seq_num;
u_long  RTSP_send_timestamp;
u_short RTSP_recv_seq_num;

#define BUFFERSIZE 16384
char    in_buffer[BUFFERSIZE];
u_short in_size;
char    out_buffer[BUFFERSIZE];
u_short out_size;

void send_reply( int err, char *addon );
void io_read(RTSP_SOCK fd);
void io_write(RTSP_SOCK fd);
int  io_write_pending();
int validate_method( char *s );


#endif /* _MSG_HANDLER_H_ */
