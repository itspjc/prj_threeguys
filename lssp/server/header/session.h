
#ifndef _SESSION_H_
#define _SESSION_H_

#include "header/socket.h"
#include "time.h"
#include "header/machdefs.h"

#define MAXSESSIONS 5
#define MAXSTREAMSESSIONS 20
#define INVALID_SESSION ((u_long)(-1))

/* session state flags */
#define SS_USE_SESSION_ID  0x00000001
#define SS_PLAY_START      0x00000002
#define SS_PLAY_FINISH     0x00000004
#define SS_PLAY_RANGE      (SS_PLAY_START | SS_PLAY_FINISH)
#define SS_SKIP_SEQUENCE   0x00000008
#define SS_CLIP_PAUSED     0x00000010
#define SESSION_NOT_SET    (u_long)-1

/* flags for individual streams */
#define STREAM_LAST_PACKET 0x0001
#define STREAM_KEY_FRAME   0x0002

/* content types that are dealt with here */
#define X_RTSP_MH                1
#define X_CONTENT_UNRECOGNIZED   2
#define WAVE_CONTENT             4
#define TEXT_CONTENT             8

/* mime types that this application supports */
#define MIME_TEXT                1
#define MIME_AUDIO               2

typedef struct sockaddr_in SOCKIN;


typedef struct PCM_header
{
   u_int16  usFormatTag;
   u_int16  usChannels;
   u_int32   ulSamplesPerSec;
   u_int16  usBitsPerSample;
   u_int16  usSampleEndianness;
} PCM_HDR;

typedef struct WaveFormatEx
{
   u_int16  wFormatTag;
   u_int16  nChannels;
   u_int32   nSamplesPerSec;
   u_int32   nAvgBytesPerSec;
   u_int16  nBlockAlign;
   u_int16  wBitsPerSample;
   u_int16  CbSize;
} WAVEFMTX;

#define _WAVE_FORMAT_PCM   1
WAVEFMTX PCM_hdr;


typedef struct SessionSettings
{
   u_long   session_id;
   SOCKIN   saddr_in;            /* ip address & port # for data channel */
   u_short  data_port;
   time_t   start;               /* play range */
   time_t   finish;
   long     max_bit_rate;
   long     ave_bit_rate;
   long     max_pkt_size;
   long     ave_pkt_size;
   int      num_interleave_pkts;
   long     duration;
   int      preroll;
   int      media_flags;
   int      num_streams;
} SSET;

typedef struct Stream
{
   struct Stream  *next;
   u_short        stream_id;
   u_short        mime_type;
   long           max_bit_rate;
   long           ave_bit_rate;
   long           max_pkt_size;
   long           ave_pkt_size;
   u_short        preroll;
   long           duration;
   u_long         xmit_interval;
   u_long        start_time;
   u_short        stream_flags;
   int            stream_fd;
   u_long         start_offset;
   u_long         byte_count;
   u_long         bytes_per_sec;
   PCM_HDR        PCM_hdr;
   char           type_data[256];
   char           filename[512];
   void           *streamer;
   u_short        seq_num;

} STREAM;

struct SESSION_STATE
{
   u_long   session_id;
   int      cur_state;
   int      flags;
   SSET     settings;
   STREAM   *streams;
   char     serverurl[256];
   char     url[256];

   int sessionlist_stream_ids[MAXSTREAMSESSIONS];
   u_short sessionlist[MAXSTREAMSESSIONS]; 	
};

/* states */
#define INIT_STATE      0
#define READY_STATE     1
#define PLAY_STATE      2

/*#endif _SESSION_H_*/
#endif
