
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#define OPEN_FLAGS O_RDONLY

#include "machdefs.h"
#include "eventloop.h"
#include "messages.h"
#include "rtsp.h"
#include "server.h"
#include "streamer.h"
#include "scheduler.h"
#include "socket.h"
#include "rtp.h"
#include "util.h"
#include "session.h"
#include "config.h"

extern struct SESSION_STATE s_state;
extern char *alloc_path_name ( char *base_path, char *file_path );


struct STREAMER *
start_stream(u_long session_id, STREAM *stream, u_short data_port)
{
   struct STREAMER *s = malloc(sizeof(struct STREAMER));
   char  *PathName;
   
   s->session_id = session_id;
   s->stream = stream;
   stream->streamer = (void *) s;   /* back reference to streamer */
   s->udp_port = data_port;
   memcpy(&s->dest, &peer, sizeof(s->dest));
   s->dest.sin_port = htons(data_port);
   PathName = alloc_path_name( BasePath, stream->filename );
   s->input_fd = open(PathName, OPEN_FLAGS);
   free( PathName );
   if(s->input_fd < 0)	/* can't open file */
   {
      free(s);
   	return 0;
   }
   stream->stream_fd = s->input_fd;
   s->input_offset = stream->start_offset;
   lseek( s->input_fd, s->input_offset, SEEK_SET );

   s->output_fd = udp_open();
   s->ssrc = 0;
   s->seq = stream->seq_num;
   s->pktsize = stream->max_pkt_size;
   s->rate = stream->xmit_interval;

   schedule_enter(inow + s->rate, s->session_id, (void *)stream_event, s);
   s->next_event_time = inow + s->rate * 2;

   return s;
}

void
stop_stream(struct STREAMER *s)
{
    schedule_remove_session(s->session_id);
}

void
resume_stream(struct STREAMER *s)
{
    s->outstanding_event_id = 
      schedule_enter(inow + s->rate, s->session_id, (void *)stream_event, s);
    s->next_event_time = inow + s->rate * 2;
}

u_char PCM2RTP_payload_type(PCM_HDR *p)
{
    u_short channels;
    u_long samplespersec;

    switch (ntohs(p->usFormatTag)) {
        case 0x1:
	  channels=ntohs(p->usChannels);
	  samplespersec=ntohl(p->ulSamplesPerSec);
	  if( samplespersec == 44100 ) {
	    if (channels == 2) {
	      return (RTP_PAYLOAD_L16_2);
	    }
	    if (channels == 1) {
	      return (RTP_PAYLOAD_L16_1);
	    }
	  }
	  return(RTP_PAYLOAD_RTSP);
        case 0x6:
    	  return (RTP_PAYLOAD_PCMA);
        case 0x7:
          return (RTP_PAYLOAD_PCMU);
    }
    return (127);
}

int
build_RTP_packet( void *RTP_pkt, char *data, int data_len, struct STREAMER *s )
{
   RTP_HDR  *r;
   RTP_OP   *o;
   char     *d;
   int      n;
   u_short BpSample; 
   
   BpSample = (ntohs(s->stream->PCM_hdr.usBitsPerSample) + 7) / 8;
   RTSP_send_timestamp += (data_len / BpSample);

   /* build the RTP header */
   r = RTP_pkt;
   r->version = 2;
   r->padding = 0;
   r->csrc_len = 0;
   r->marker = 0;
   r->payload = PCM2RTP_payload_type(&s->stream->PCM_hdr);

   r->seq_no = htons(RTSP_send_seq_num++);
   r->timestamp = htonl(RTSP_send_timestamp);
   r->ssrc = s->session_id;

   n = sizeof(RTP_HDR);
   if ( data_len )
   {
      r->extension = 0;
      d = ((char *) RTP_pkt + n);
      if(BpSample==2) {
	/* blindly assume that WAV files are stored little endian */
	swap_word((unsigned short *) data, (data_len/2));
      }
      /* this copy is not efficient, but it gets the job done. */
      memcpy( d, data, data_len );
   }
   else
   {
      r->extension = 1;
      o = (RTP_OP *) ((char *) RTP_pkt + n);
      o->op_code = htons( RTP_OP_PACKETFLAGS );
      o->op_code_data_length = htons( RTP_OP_CODE_DATA_LENGTH );
      o->op_code_data [0] = htonl( RTP_FLAG_LASTPACKET );
      n += sizeof(RTP_OP);
      d = ((char *) RTP_pkt + n);
   }

   /* data_len in the packet is self inclusive. */
   return( n + data_len );
}

void
stream_event(struct STREAMER *s)
{
   static char    buffer[16384];
   static char    RTP_pkt [sizeof(buffer) + sizeof(RTP_HDR) +
                           sizeof(RTP_OP) + sizeof(RTP_DATA)];
   static int     in_stream_event = 0;
   int            n, on;

   assert( !in_stream_event );
   if ( in_stream_event )
      return;
   in_stream_event = 1;

   assert( (s->pktsize + (sizeof(RTP_HDR) + 2)) <= sizeof(buffer) );
   n = read( s->input_fd, buffer, s->pktsize );
   if ( n != -1 )
      s->input_offset += n;
   else
      n = 0;   /* on error condition make like it's a last packet */
   on = build_RTP_packet( RTP_pkt, buffer, n, s );
   dgram_sendto(s->output_fd, RTP_pkt, on, 0, (struct sockaddr *)&s->dest,
                sizeof(s->dest));

   if ( n == s->pktsize )
   {
      s->outstanding_event_id = schedule_enter(s->next_event_time,
                                    s->session_id, (void *)stream_event, s);
      s->next_event_time += s->rate;
   }
   else
   {  /* this was a last packet condition so send an RTP pkt saying so. */
      on = build_RTP_packet( RTP_pkt, buffer, 0, s );
      dgram_sendto(s->output_fd, RTP_pkt, on, 0, (struct sockaddr *)&s->dest,
                   sizeof(s->dest));
            
      if ( s_state.cur_state == PLAY_STATE )
         s_state.cur_state = READY_STATE;
      s->input_offset = s->stream->start_offset;
      lseek( s->input_fd, s->input_offset, SEEK_SET );
   }

   in_stream_event = 0;
}
