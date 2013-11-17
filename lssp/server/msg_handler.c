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



TKN Methods [] =
{
    {PLAY_TKN, RTSP_PLAY_METHOD},
    {PAUSE_TKN, RTSP_PAUSE_METHOD},
    {GET_TKN, RTSP_GET_METHOD},
    {SETUP_TKN, RTSP_SETUP_METHOD},
    {REDIRECT_TKN, RTSP_REDIRECT_METHOD},
    {SESSION_TKN, RTSP_SESSION_METHOD},
    {HELLO_TKN, RTSP_HELLO_METHOD},
    {CLOSE_TKN, RTSP_CLOSE_METHOD},
    {RECORD_TKN, RTSP_RECORD_METHOD},
    {GET_PARAM_TKN, RTSP_GET_PARAM_METHOD},
    {SET_PARAM_TKN, RTSP_SET_PARAM_METHOD},
    {EXT_METHOD_TKN, RTSP_EXTENSION_METHOD},
    {0, -1}
};

void
msg_handler()
{
    char buffer[BLEN]; //4096
    int  opcode;
    u_short status;
    status = 0;

    /* in_buffer -> buffer */

    u_short llen;

    // in_buffer 가 가득찰때까지 기다려야 하는 부분을 만들어야 함

    llen = strcspn (in_buffer, "\r\n"); // 한 줄의 길이를 구함
    
    while(llen != 0)
    {
        memcpy (buffer, in_buffer, llen); // 먼저 buffer 에 복사
        opcode = validate_method(buffer); //parse.c:371 읽은 RTSP를 분석한다. 
        handle_event(opcode, status, buffer); //server.c:915
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
    else 
    {
        out_size = 0;
        printf("Write to Client socket successfully done.\n");
    }
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

int
validate_method( char *s )
{
    TKN *m;
    char directive [32];
    char host [256];
    char version [32];
    int pcnt; /* parameter count */

    *directive = *host = '\0';

    /* pcnt = 3 이면 RTSP 첫번째 줄을 나타냄 */
    pcnt = sscanf( s, "%31s %255s %31s", directive, host, version);
    
    if (pcnt != 3)
    {
        discard_msg(); /* remove remainder of message from in_buffer */
        return(-2);
    }
    

    /* 어떤 Directive인지 찾는다 */
    for (m = Methods; m->code != -1; m++)
        if (!strcmp( m->token, directive ))
            break;

    // 현재 m 은 해당 token 을 가지고 있음
    
    
    if ( m->code == -1 )
    {
        discard_msg(); /* remove remainder of message from in_buffer */
        printf("No Existed Directive.\n");
    }

    printf("%s, %d \n", s, m->code); // 읽은 한줄이랑 코드네임 출력해본다.
    return( m->code ); //opcode 리턴
}


