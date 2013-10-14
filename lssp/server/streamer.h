
#ifndef _STREAMER_H_
#define _STREAMER_H_

#include "session.h"

struct STREAMER
{
   u_long   session_id;
   STREAM   *stream;
   u_short  udp_port;
   int      output_fd;
   struct sockaddr_in dest;
   int      input_fd;
   u_long   input_offset;

   int      rate;
   int      pktsize;

   int      seq;
   int      next_event_time;
   int      outstanding_event_id;

   u_char   payload_type;
   u_long   ssrc;
};

struct STREAMER *start_stream(u_long session_id, STREAM *stream, u_short data_port);
void stop_stream(struct STREAMER *s);
void resume_stream(struct STREAMER *s);
void stream_event(struct STREAMER *s);
u_char PCM2RTP_payload_type(PCM_HDR *p);

#endif  /* _STREAMER_H_ */
