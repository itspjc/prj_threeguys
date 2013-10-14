
#ifndef _PARSE_H_
#define _PARSE_H_

#include <socket.h>


#define HDRBUFLEN   4096
#define TOKENLEN    100

#define SETUP_TKN       "SETUP"
#define REDIRECT_TKN    "REDIRECT"
#define PLAY_TKN        "PLAY"
#define PAUSE_TKN       "PAUSE"
#define SESSION_TKN     "SESSION"
#define RECORD_TKN      "RECORD"
#define EXT_METHOD_TKN  "EXT-"

#define HELLO_TKN       "OPTIONS"
#define GET_TKN         "DESCRIBE"
#define GET_PARAM_TKN   "GET_PARAMETER"
#define SET_PARAM_TKN   "SET_PARAMETER"
#define CLOSE_TKN       "TEARDOWN"


/* method codes */
#define RTSP_SETUP_METHOD     0
#define RTSP_GET_METHOD       1
#define RTSP_REDIRECT_METHOD  2
#define RTSP_PLAY_METHOD      3
#define RTSP_PAUSE_METHOD     4
#define RTSP_SESSION_METHOD   5
#define RTSP_HELLO_METHOD     6
#define RTSP_RECORD_METHOD    7
#define RTSP_CLOSE_METHOD     8
#define RTSP_GET_PARAM_METHOD 9
#define RTSP_SET_PARAM_METHOD 10
#define RTSP_EXTENSION_METHOD 11

#define RTSP_SETUP_RESPONSE      100
#define RTSP_GET_RESPONSE        101
#define RTSP_REDIRECT_RESPONSE   102
#define RTSP_PLAY_RESPONSE       103
#define RTSP_PAUSE_RESPONSE      104
#define RTSP_SESSION_RESPONSE    105
#define RTSP_HELLO_RESPONSE      106
#define RTSP_RECORD_RESPONSE     107
#define RTSP_CLOSE_RESPONSE      108
#define RTSP_GET_PARAM_RESPONSE  109
#define RTSP_SET_PARAM_RESPONSE  110
#define RTSP_EXTENSION_RESPONSE  111

typedef struct tokens
{
   char  *token;
   int   code;
} TKN;

char *get_stat( int code );
char * get_method( int code );
void remove_msg( int len );
void discard_msg( void );
int valid_response_msg( char *s, u_short *seq_num, u_short *status,
                        char *msg );
int validate_method( char *s );
void get_msg_len( int *hdr_len, int *body_len );
char *get_int( char *buf, int hlen, char *tkn, char *sep, int *nint, long *nlong );
char *get_string( char *buf, int hlen, char *tkn, char *sep );
void chk_buff_size( int len );
void str_to_lower( char *s, int l );
int set_stream_settings( char *b, int len, struct SESSION_STATE *state );
void free_streams( struct SESSION_STATE *state );


#endif  /* _PARSE_H_ */
