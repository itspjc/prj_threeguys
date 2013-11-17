#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "header/machdefs.h"
#include "header/msg_handler.h"
#include "header/socket.h"
#include "header/util.h"
#include "header/parse.h"
#include "header/rtsp.h"
/*
extern u_short  in_size;
extern char      in_buffer[];
*/
extern const char sContentLength[];

extern void terminate(int errorcode);
extern int valid_response_msg( char *s, u_short *seq_num, u_short *status, char *msg );
extern int validate_method( char *s );
extern void send_generic_reply( int err );
extern void handle_event( int event, int status, char *buf );
extern void terminate(int errorcode);

u_short RTSP_last_request;       /* code for last request message sent */
u_short RTSP_send_seq_num = 1;
u_short RTSP_recv_seq_num = (u_short) -1;
char    in_buffer[BUFFERSIZE];
u_short in_size = 0;
char    out_buffer[BUFFERSIZE];
u_short out_size = 0;

void
msg_handler()
{
    char buffer[BLEN]; //4096
    //char msg [STATUS_MSG_LEN]; /* for status string on response messages */
    int  opcode;
    //int  r;
    //u_short seq_num;
    u_short status;
    status = 0;

    while( RTSP_read( buffer, BLEN ) ) // buffer 변수에 in_buffer의 내용이 복사된다.
    {
        /* check for response message.
         *
         * valid_response_msg
         * 어떤 메세지인지 보는 것
         * 지금은 별로 필요없어서 주석처리 해놓음
         * Return value :
         * Request from client - 0, Response from server - 1
         */
        //r = valid_response_msg( buffer, &seq_num, &status, msg ); //parse.c

        /* validate_method :
         * 1. 버퍼 내용을 읽어서 실제적인 파싱이 이루어지는 곳
         * 2. 리턴되는 opcode를 통해 어떤 명령을 수행해야 될지 handle_event()가 결정
         */

        opcode = validate_method(buffer); //parse.c
        
        /* 프로토콜의 유효성을 검사하는 부분인데 지금은 볼필요 없음
        if ( !r )
        {
            opcode = validate_method( buffer );
            if ( opcode < 0 )
            {
                if ( opcode == -1 )
                {
                    assert( sizeof(msg) > 255 );
                    if ( !sscanf( buffer, " %255s ", msg ) )
                        strcpy( msg, "[no method encountered]" );
                    printf( "Method requested was invalid.  Message discarded.  "
                              "Method = %s\n", msg );
                }
                else if ( opcode == -2 )
                    printf( "Bad request line encountered.  Expected 4 valid "
                              "tokens.  Message discarded.\n" );
                return;
            }
            status = 0;
        }
        else if ( r != -1 )
        {
            opcode = RTSP_last_request + 100;
            if ( status > 202 )
                printf( "Response had status = %d.\n", status );
        }
        else
            return;
        */
        handle_event( opcode, status, buffer ); //server.c:915
    }
}


void
io_write(RTSP_SOCK fd)
{
    int n;

    if ((n = tcp_write(fd, out_buffer, out_size)) < 0)
    {
        printf( "PANIC: tcp_write() error.\n" );
        terminate(-1);
    }
    else {
        out_size = 0;
        printf("Write to Client socket successfully done.\n");
}

/* Fill in_buffer from network, used from eventloop only */
void
io_read(RTSP_SOCK fd)
{
    int n;

	// in_size 변수는 0으로 초기화되어 있다.
    // n : btyes that read from client socket.
    if ((n = tcp_read(fd, &in_buffer[in_size], sizeof(in_buffer) - in_size)) < 0)
    {
		printf( "PANIC: tcp_read() error.\n" );
		terminate(-1);
    }
    else
    	in_size += n;

    if(!n)
	    terminate(0);
}

int
io_write_pending()
{
    return out_size;
}

