
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "machdefs.h"
#include "rtsp.h"
#include "messages.h"
#include "parse.h"
#include "util.h"

extern void terminate(int errorcode);

u_short RTSP_last_request;       /* code for last request message sent */
u_short RTSP_send_seq_num = 1;
u_short RTSP_recv_seq_num = (u_short) -1;
char    in_buffer[BUFFERSIZE];
u_short in_size = 0;
char    out_buffer[BUFFERSIZE];
u_short out_size = 0;


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
    else
        out_size = 0;
}

/* Fill in_buffer from network, used from eventloop only */
void
io_read(RTSP_SOCK fd)
{
    int n;

    if ((n = tcp_read(fd, &in_buffer[in_size], sizeof(in_buffer) - in_size)) < 0)
    {
      printf( "PANIC: tcp_read() error.\n" );
    	terminate(-1);
    }
    else
    	in_size += n;

    if(!n)
	    terminate(0);	/* XXX */
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
