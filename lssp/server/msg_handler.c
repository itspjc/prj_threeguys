
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "machdefs.h"
#include "msg_handler.h"
#include "socket.h"
#include "util.h"
#include "messages.h"
#include "parse.h"

extern u_short  in_size;
extern char     in_buffer[];
extern const char sContentLength[];

extern void terminate(int errorcode);
extern int valid_response_msg( char *s, u_short *seq_num, u_short *status, char *msg );
extern int validate_method( char *s );
extern void send_generic_reply( int err );
extern void handle_event( int event, int status, char *buf );

int
full_msg_rcvd( void )
{
   int   eomh;    /* end of message header found */
   int   mb;      /* message body exists */
   int   tc;      /* terminator count */
   int   ws;      /* white space */
   int   ml;      /* total message length including any message body */
   int   bl;      /* message body length */
   char  c;       /* character */

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
         return( 0 );      /* haven't received the entire message yet. */
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
         eomh = 1;      /* must be the end of the message header */
      ml += tc + ws;

      if ( eomh )
      {
         ml += bl;   /* add in the message body length, if collected earlier */
         if ( ml <= in_size )
            break;   /* all done finding the end of the message. */
      }

      if ( ml >= in_size )
         return( 0 );      /* haven't received the entire message yet. */

      /*
       * check first token in each line to determine if there is
       * a message body.
       */
      if ( !mb )     /* content length token not yet encountered. */
      {
         if ( !strncasecmp( &in_buffer [ml], sContentLength,
                                                  strlen(sContentLength) ) )
         {
            mb = 1;     /* there is a message body. */
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

int
RTSP_read( char *buffer, int buflen )
{
   u_short     llen;                /* line length */


   if (!in_size)
      return( 0 );

   if ( !full_msg_rcvd() )
      return( 0 );

   /* transfer bytes from first line of message header for parsing. */
   llen = strcspn( in_buffer, "\r\n" );

   if (!llen) {
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
   buffer [llen] = '\0';   /* terminate it for further parsing */

   
   return( 1 );      /* a full message is ready to be handled */
}


void
msg_handler()
{
   char     buffer[BLEN];
   char     msg [STATUS_MSG_LEN]; /* for status string on response messages */
   int      opcode;
   int      r;
   u_short  seq_num;
   u_short  status;

   while( RTSP_read( buffer, BLEN ) )
   {

      /* check for response message. */
      r = valid_response_msg( buffer, &seq_num, &status, msg );
      if ( !r )
      {  /* not a response message, check for method request. */
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
         return;     /* response message was not valid, ignore it. */

      handle_event( opcode, status, buffer );
   }
}
