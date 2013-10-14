
#ifndef _MESSAGES_H_
#define _MESSAGES_H_


#include "socket.h"
#include "session.h"

u_short RTSP_last_request;       /* code for last request message sent */
u_short RTSP_send_seq_num;
u_long  RTSP_send_timestamp;
u_short RTSP_recv_seq_num;

#define BUFFERSIZE 16384
char    in_buffer[BUFFERSIZE];
u_short in_size;
char    out_buffer[BUFFERSIZE];
u_short out_size;

void add_time_stamp( char * b, int crlf );
void add_play_range( char *b, struct SESSION_STATE *state );
void add_session_id( char *b, struct SESSION_STATE *state );
void send_reply( int err, char *addon );
void io_read(RTSP_SOCK fd);
void io_write(RTSP_SOCK fd);
int  io_write_pending();
void bread(char *buffer, u_short len);
void bwrite(char *buffer, u_short len);


#endif /* _MESSAGES_H_ */
