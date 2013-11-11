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


int
full_msg_rcvd( void )
{
    int eomh;     /* end of message header found */
    int mb;        /* message body exists */
    int tc;        /* terminator count */
    int ws;        /* white space */
    int ml;        /* total message length including any message body */
    int bl;        /* message body length */
    char c;         /* character */

    /*
     * return 0 if a full RTSP message is NOT present in the in_buffer yet.
     * return 1 if a full RTSP message is present in the in_buffer and is
     * ready to be handled.
     * terminate on really ugly cases.
     */
    eomh = mb = ml = bl = 0;
    while ( ml <= in_size )
    {
        /* look for eol. */
        ml += strcspn( &in_buffer [ml], "\r\n" );
        if ( ml > in_size )
            return( 0 );        /* haven't received the entire message yet. */
        /*
         * work through terminaters and then check if it is the
         * end of the message header.
         */
        tc = ws = 0;
        while ( !eomh && ((ml + tc + ws) < in_size) )
        {
            c = in_buffer [ml + tc + ws];
            if ( c == '\r' || c == '\n' )
                tc++;
            else if ( (tc < 3) && ((c == ' ') || (c == '\t')) )
                ws++; /* white space between lf & cr is sloppy, but tolerated. */
            else
                break;
        }
        /*
         * cr,lf pair only counts as one end of line terminator.
         * Double line feeds are tolerated as end marker to the message header
         * section of the message.  This is in keeping with RFC 2068,
         * section 19.3 Tolerant Applications.
         * Otherwise, CRLF is the legal end-of-line marker for all HTTP/1.1
         * protocol compatible message elements.
         */
        if ( (tc > 2) || ((tc == 2) && (in_buffer [ml] == in_buffer [ml + 1])) )
            eomh = 1;        /* must be the end of the message header */
        ml += tc + ws;

        if ( eomh )
        {
            ml += bl;    /* add in the message body length, if collected earlier */
            if ( ml <= in_size )
                break;    /* all done finding the end of the message. */
        }

        if ( ml >= in_size )
            return( 0 );        /* haven't received the entire message yet. */

        /*
         * check first token in each line to determine if there is
         * a message body.
         */
        if ( !mb )      /* content length token not yet encountered. */
        {
            if ( !strncasecmp( &in_buffer [ml], sContentLength,
                                                                  strlen(sContentLength) ) )
            {
                mb = 1;      /* there is a message body. */
                ml += strlen(sContentLength);
                while ( ml < in_size )
                {
                    c = in_buffer [ml];
                    if ( (c == ':') || (c == ' ') )
                        ml++;
                    else
                        break;
                }

                if ( sscanf( &in_buffer [ml], "%d", &bl ) != 1 )
                {
                    printf("PANIC: invalid ContentLength encountered in message.");
                    terminate( -1 );
                }
            }
        }
    }

    return( 1 );
}

/* Parsing Only RTSP Header (first line) */ 
int
RTSP_read( char *buffer, int buflen )
{
    u_short llen; /* line length */

    if (!in_size) //하나도 아직 버퍼에 읽힌게 없다면
        return( 0 );

    if ( !full_msg_rcvd() )
        return( 0 );

    /* transfer bytes from first line of message header for parsing. */
    llen = strcspn( in_buffer, "\r\n" );

    if (!llen) { // 아마 메세지 들어올때까지 여기에 머물러 있으라는 말인가...
      remove_msg(2);
      return RTSP_read(buffer, buflen);
    }

    if (llen > in_size)
    {
        printf( "PANIC:  message header in buffer is invalid.\n" );
        terminate( -1 );
    }

    if ( llen >= buflen )
    {
        printf( "PANIC:  message header in buffer is too large.\n" );
        terminate( -1 );
    }
        
    memcpy( buffer, in_buffer, llen );
    buffer [llen] = '\0';    /* terminate it for further parsing */

    
    return( 1 );        /* a full message is ready to be handled */
}


void
msg_handler()
{
    char buffer[BLEN];
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



/* messages.c 통합 */

void
add_time_stamp( char * b, int crlf )
{
   struct tm   *t;
   time_t      now;

   now = time(NULL);
   t = gmtime(&now);
   strftime( b + strlen( b ), 38, "Date: %d %b %Y %H:%M:%S GMT\n", t );
   if ( crlf )
      strcat( b, "\r\n" );   /* add a message header terminator (CRLF) */
}

void
set_time( char *b, time_t t )
{
   struct tm *time;
   
   /* convert time structure to smpte format */
   time = localtime( &t );
   strftime( b + strlen( b ), 12, "%H:%M:%S", time );
}                               

void
add_play_range( char *b, struct SESSION_STATE *state )
{
   strcat ( b, "Range:smpte" );
   set_time ( b, state->settings.start );
   strcat ( b, "-" );
   if ( state->flags & SS_PLAY_FINISH )
      set_time ( b, state->settings.finish );
   strcat ( b, "\n" );
}

void
add_session_id( char *b, struct SESSION_STATE *state )
{
   sprintf( b + strlen( b ), "Session:%ld\n", state->settings.session_id );
}

void
send_reply( int err, char *addon )
{
   u_short  len;
   char     *b;

   if ( addon )
      len = 256 + strlen(addon);
   else
      len = 256;
   b = malloc( len );
   assert( b );
   if ( !b )
   {
      printf( "PANIC: memory allocation error in send_reply().\n" );
      terminate(-1);
   }
   sprintf( b, "%s %d %d %s\n", RTSP_VER, err, RTSP_recv_seq_num,
            get_stat( err ));
   add_time_stamp ( b, 0 );
   assert( len > (strlen(b) + ((addon) ? strlen(addon) : 0)) );
   if ( addon )
      strcat( b, addon );
   strcat( b, "\n" );
   bwrite( b, (u_short) strlen( b ) );
   free( b );
}




void
bread(char *buffer, u_short len)
{
   if ( len > in_size )
   {
      printf( "PANIC: bread() request exceeds buffered data.\n" );
      terminate(-1);
   }

   memcpy (buffer, in_buffer, len);
   in_size -= len;
   if ( in_size && len )    /* discard the message from the in_buffer. */
      memmove( in_buffer, &in_buffer [len], in_size ); 
}

/* Write out_buffer to network, used from eventloop only */
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

void
bwrite(char *buffer, u_short len)
{
   if ( (out_size + len) > (int) sizeof(out_buffer) )
   {
      printf( "PANIC: not enough free space to buffer send message.\n" );
      terminate(-1);
   }
   memcpy (&out_buffer[out_size], buffer, len);
   out_size += len;
}
