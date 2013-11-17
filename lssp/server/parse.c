
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>

#include "header/machdefs.h"
#include "header/msg_handler.h"
#include "header/parse.h"
#include "header/util.h"
#include "header/session.h"

extern void terminate(int errorcode);

/*WAVEFMTX PCM_hdr;*/
char  *sInvld_method = "Invalid Method";

/* message header keywords */
const char  sContentLength[] = "Content-Length";
const char  sAccept[] = "Accept";
const char  sAllow[] = "Allow";
const char  sBlocksize[] = "Blocksize";
const char  sContentType[] = "Content-Type";
const char  sDate[] = "Date";
const char  sRequire[] = "Require";
const char  sTransportRequire[] = "Transport-Require";
const char  sSequenceNo[] = "SequenceNo";
const char  sSeq[] = "Seq";
const char  sStream[] = "Stream";
const char  sSession[] = "Session";


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


TKN Status [] =
{
    {"Continue", 100},
    {"OK", 200},
    {"Created", 201},
    {"Accepted", 202},
    {"Non-Authoritative Information", 203},
    {"No Content", 204},
    {"Reset Content", 205},
    {"Partial Content", 206},
    {"Multiple Choices", 300},
    {"Moved Permanently", 301},
    {"Moved Temporarily", 302},
    {"Bad Request", 400},
    {"Unauthorized", 401},
    {"Payment Required", 402},
    {"Forbidden", 403},
    {"Not Found", 404},
    {"Method Not Allowed", 405},
    {"Not Acceptable", 406},
    {"Proxy Authentication Required", 407},
    {"Request Time-out", 408},
    {"Conflict", 409},
    {"Gone", 410},
    {"Length Required", 411},
    {"Precondition Failed", 412},
    {"Request Entity Too Large", 413},
    {"Request-URI Too Large", 414},
    {"Unsupported Media Type", 415},
    {"Bad Extension", 420},
    {"Invalid Parameter", 450},
    {"Parameter Not Understood", 451},
    {"Conference Not Found", 452},
    {"Not Enough Bandwidth", 453},
    {"Session Not Found", 454},
    {"Method Not Valid In This State", 455},
    {"Header Field Not Valid for Resource", 456},
    {"Invalid Range", 457},
    {"Parameter Is Read-Only", 458},
    {"Internal Server Error", 500},
    {"Not Implemented", 501},
    {"Bad Gateway", 502},
    {"Service Unavailable", 503},
    {"Gateway Time-out", 504},
    {"RTSP Version Not Supported", 505},
    {"Extended Error:", 911},
    {0, -1}
};


char *
get_stat( int code )
{
    TKN    *s;
    char  *r;
    
    for ( r = 0, s = Status; s->code != -1; s++ )
        if ( s->code == code )
            return( s->token );

    printf( "??? Invalid Error Code used." );
    return( sInvld_method );
}

char *
get_method( int code )
{
    TKN    *m;
    char  *r;
    
    for ( r = 0, m = Methods; m->code != -1; m++ )
        if ( m->code == code )
            return( m->token );

    printf( "??? Invalid Method Code used." );
    return( sInvld_method );
}

void
get_msg_len( int *hdr_len, int *body_len )
{
    int    eom;      /* end of message found */
    int    mb;        /* message body exists */
    int    tc;        /* terminator count */
    int    ws;        /* white space */
    int    ml;        /* total message length including any message body */
    int    bl;        /* message body length */
    char  c;         /* character */
    char  *p;

    eom = mb = ml = bl = 0;
    while ( ml <= in_size )
    {
        /* look for eol. */
        ml += strcspn( &in_buffer [ml], "\r\n" );
        if ( ml > in_size )
            break;
        /*
         * work through terminaters and then check if it is the
         * end of the message header.
         */
        tc = ws = 0;
        while ( !eom && ((ml + tc + ws) < in_size) )
        {
            c = in_buffer [ml + tc + ws];
            if ( c == '\r' || c == '\n' )
                tc++;
            else if ( (tc < 3) && ((c == ' ') || (c == '\t')) )
                ws++; /* white space between lf & cr is sloppy, but tolerated. */
            else
                break;
        }
        if ( (tc > 2) || ((tc == 2) && (in_buffer [ml] == in_buffer [ml + 1])) )
            eom = 1;             /* must be the end of the message header */
        ml += tc + ws;

        if ( eom )
        {
            ml += bl;        /* add in the message body length */
            break;            /* all done finding the end of the message. */
        }
        if ( ml >= in_size )
            break;
        if ( !mb )      /* content length token not yet encountered. */
        {
            if ( !strncasecmp( &in_buffer [ml], sContentLength,
                                                                  sizeof(sContentLength)-1 ) )
            {
                mb = 1;      /* there is a message body. */
                ml += sizeof(sContentLength)-1;
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
                    printf("ALERT: invalid ContentLength encountered in message.");
                    terminate( -1 );
                }
            }
        }
    }

    if ( ml > in_size )
    {
        printf( "PANIC: buffer did not contain the entire RTSP message." );
        terminate( -1 );
    }

    *hdr_len = ml - bl;
    for ( tc = in_size - ml, p = &in_buffer [ml];
            tc && (*p == '\0'); p++, bl++, tc-- );
    *body_len = bl;
}

void
remove_msg( int len )
{
    in_size -= len;
    if ( in_size && len )     /* discard the message from the in_buffer. */
        memmove( in_buffer, &in_buffer [len], in_size ); 
}

void
discard_msg( void )
{
    int    hl;        /* message header length */
    int    bl;        /* message body length */
    
    get_msg_len( &hl, &bl );
    remove_msg( hl + bl );     /* now discard the entire message. */
}

void
str_to_lower( char *s, int l )
{
    while ( l )
    {
        *s = tolower( *s );
        s++;
        l--;
    }
}

char *
get_string( char *buf, int hlen, char *tkn, char *sep )
{
    char      b [HDRBUFLEN];
    char      t [TOKENLEN];  /* temporary buffer for protected string constants. */
    int        tlen;             /* token length */
    int        n;
    u_long    idx;
    char      *p;

    /* prepare the strings for comparison */
    tlen = strlen( tkn );
    assert( hlen );
    assert( sizeof( b ) > (u_long) hlen );
    assert( sizeof( t ) > (u_long) tlen );
    strncpy( b, buf, hlen );    /* create a null terminated string */
    b [hlen] = '\0';              /* make sure the string is terminated */
    strcpy( t, tkn );             /* create a temporary copy. */
    /* transform to lower case before doing string search. */
    str_to_lower( b, hlen );
    str_to_lower( t, tlen );

    /* now see if the header setting token is present in the message header */
    if ( (p = strstr( b, t )) ) {
        if ( (n = strspn( p + tlen, sep )) || (!strlen(sep)))
        {
            p += tlen + n;
            idx = p - b;
            p = buf + idx;
        }
        else
            p = 0;            /* there was not a valid separater. */
    }

    return( p );
}

char *
get_int( char *buf, int hlen, char *tkn, char *sep, int *nint, long *nlong )
{
    char      b [HDRBUFLEN];
    char      t [TOKENLEN];  /* temporary buffer for protected string constants. */
    int        tlen;             /* token length */
    int        n;
    u_long    idx;
    char      *p;

    /* prepare the strings for comparison */
    tlen = strlen( tkn );
    assert( hlen );
    assert( sizeof( b ) > (u_long) hlen );
    assert( sizeof( t ) > (u_long) tlen );
    strncpy( b, buf, hlen );    /* create a null terminated string */
    strcpy( t, tkn );             /* create a temporary copy. */

    str_to_lower( b, hlen );
    str_to_lower( t, tlen );

    if ( (p = strstr( b, t )) ) {
        if ( (n = strspn( p + tlen, sep )) )
        {
            p += tlen + n;
            if ( nint )
                n = sscanf( p, "%d", nint );

            if ( n && nlong )
                n = sscanf( p, "%ld", nlong );

            if ( n == 1 )
            {
                idx = p - b;
                p = buf + idx;
            }
            else
                p = 0;            /* just ignore bad value. */
        }
        else
            p = 0;                /* there was not a valid separater. */
    }

    return( p );
}

int
valid_response_msg( char *s, u_short *seq_num, u_short *status, char *msg )
{
    char ver [32];

    char directive [32];
    char host[256];
    char version[32];

    unsigned int stat;
    unsigned int seq;
    int pcnt; /* parameter count */

    *ver = *msg = '\0';
    
    /* assuming "stat" may not be zero (probably faulty) */
    stat = 0;
    seq = 0;
    
    
    pcnt = sscanf( s, "%31s %255s %31s", directive, host, version );
    
    if (pcnt == 3) {
        printf("***************************\n");
        printf("directvie : %s\n", directive);
        printf("host : %s\n", host);
        printf("version : %s\n", version);
        printf("***************************\n");
    }
    else {
        printf("%s\n", s);
    }

    /* check for a valid response token. */
    if (strncasecmp (directive, "rtsp://", 7 ))
        return( 0 );  /* not a response message */

    /* confirm that at least the version, status code and sequence are there. */
    if ( pcnt == 3)
        return( 0 );  /* not a response message */


    *status = stat;
    return( 1 );
}

