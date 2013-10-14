
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#include "machdefs.h"
#include "eventloop.h"
#include "interface.h"
#include "messages.h"
#include "scheduler.h"
#include "player.h"
#include "rtsp.h"
#include "session.h"
#include "socket.h"
#include "msg_handler.h"
#include "parse.h"
#include "util.h"
#include "rtp.h"

extern const char sContentType[];
extern const char sSession[];

void            handle_event(int event, int status, char *buf);
extern void     terminate(int errorcode);
extern int      Quit;

#define BUFSIZE 16384

/* internal request ids */
#define REQUEST_FORMAT_DESCRIPTOR	200
#define REQUEST_AUDIO_ANNOTATIONS	201

/* user command event codes */
#define CMD_OPEN		10000
#define CMD_GET		10001
#define CMD_PLAY		10002
#define CMD_PAUSE		10003
#define CMD_CLOSE		10004

struct SESSION_STATE p_state;
extern int      debug_toggle;
int             udp_fd = 0;
int             authenticated = 0;
int             curr_payload_type;

u_long          inow;		/* Internal time format (ms) */
int             DescriptionFormat;
char           OpenFileName[100];

void
init_player()
{
  assert(offsetof(RTP_HDR, ssrc) == 8);
  schedule_init();
  udp_fd = 0;
  p_state.flags = 0;
  p_state.url[0] = '\0';
  p_state.serverurl[0] = '\0';
  p_state.cur_state = INIT_STATE;
  p_state.streams = 0;
}

void
assign_udpfd(int fd)
{
  assert(fd <= MAX_FDS);
  udp_fd = fd;
  readers[fd] = udp_data_recv;
}

void
clear_udpfd(void)
{
  int             i;

  for (i = 2; i < MAX_FDS; i++)
    readers[i] = 0;
  if (udp_fd)
    udp_close(udp_fd);
  udp_fd = 0;
}


int
end_of_streams(STREAM * s)
{
  int             r = 1;

  while (s) {
    if (!(s->stream_flags & STREAM_LAST_PACKET)) {
      r = 0;
      break;
    }
    s = s->next;
  }

  return (r);
}

int
base64_to_bin(u_char c)
{
  int             r;

  if ((c >= 'A') && (c <= 'Z'))
    r = (int) (c - 'A');
  else if ((c >= 'a') && (c <= 'z'))
    r = (int) (c - 'a') + 26;
  else if ((c >= '0') && (c <= '9'))
    r = (int) (c - '0') + 52;
  else if (c == '+')
    r = 62;
  else if (c == '/')
    r = 63;
  else
    r = -1;			/* not a base64 character */

  return (r);
}


int
decode_base64(u_char * o, char *i, int len)
{
  int             n;
  int             v;
  u_int32         a = 0;	/* accumulated value */
  int             s = 0;	/* shift register */

  for (n = 0; n < len; i++) {
    v = base64_to_bin(*i);
    if (v == -1)
      return (0);		/* hit an invalid base64 character */

    a <<= 6;			/* accumulate binary value */
    s += 6;
    a |= (u_int32) v;
    if (s >= 8) {
      s -= 8;
      v = a >> s;
      *o++ = (u_char) (v & 0xFFL);
      n++;
    }
  }

  return (1);
}


int
set_one_stream(char *b, int len, STREAM * s)
{
  char           *p, *n, *e;	/* buffer pointers */
  int             sl;		/* search limit */
  int             sdl;		/* stream descriptor length */
PCM_HDR         pcm_header;	/* header description from opaque data */


  if (DescriptionFormat == DESCRIPTION_MH_FORMAT) {
    /* locate the beginning of the Media Stream Descriptor. */
    if ((p = get_string(b, len, "<MediaStream", " \t\r\n")) == 0) {
      printf("ALERT: Missing MediaStream description.\n");
      return (0);
    }
    sl = len - (int) (p - b);	/* adjust search length for new reference. */

    /* locate the end of the Media Stream descriptor */
    if ((e = get_string(p, sl, ">", " \t\r\n")) == 0) {
      printf("ALERT: Missing Media Stream description termination.\n");
      return (0);
    }
    sdl = (int) (e - b);	/* length of stream description */

    if (!get_int(p, sl, "StreamID", " =\t", (int *) &s->stream_id, 0)) {
      printf("ALERT: Missing StreamID from Media Stream description.\n");
      return (0);
    }
    /* set mime type */
    if ((n = get_string(p, sl, "MimeType", " =\t")) == 0) {
      printf("ALERT: Missing MimeType from Media Container.\n");
      return (0);
    }

    if (strncasecmp(n, "plain/text", 4) == 0)
      s->mime_type = MIME_TEXT;
    else if (strncasecmp(n, "audio/x-pn-wav", 14) == 0)
      s->mime_type = MIME_AUDIO;
    else {
      printf("ALERT: MimeType not supported.  Player supports only `Text' or "
	     "'audio/x-pn-wav'.\n");
      return (0);
    }

    if (!get_int(p, sl, "MaxBitRate", " =\t", 0, &s->max_bit_rate)) {
      printf("ALERT: MaxBitRate not found in <MediaStream> descriptor.");
      return (0);
    }
    if (!get_int(p, sl, "MaxPktSize", " =\t", 0, &s->max_pkt_size)) {
      printf("ALERT: MaxBitRate not found in <MediaStream> descriptor.");
      return (0);
    }
  } else {
    /* SDP format. */
    
    /* locate the beginning of the Media Stream Descriptor. */
    if ((p = get_string(b, len, "m=", "")) == 0) {
      printf("ALERT: Missing MediaStream description.\n");
      return (0);
    }
    p -= 2;
    sl = len - (int) (p - b);	/* adjust search length for new reference. */

    /* locate the end of the Media Stream descriptor */
    if ((e = get_string(p + 2, sl - 2, "m=", "")) == 0) {
      e = p + sl - 1;
    } else
      e -= 2;

    sdl = (int) (e - b);	/* length of stream description */

    /* set mime type */
    if ((n = get_string(p, sl, "m=", "")) == 0) {
      printf("ALERT: Missing MimeType from Media Container.\n");
      return (0);
    }
    /* for now I only accept text or audio MIME types. */
    if (strncasecmp(n, "other_media_type", 16) == 0)
      s->mime_type = MIME_TEXT;
    else if (strncasecmp(n, "audio 0 RTP/AVP", 15) == 0)
      s->mime_type = MIME_AUDIO;
    else {
      printf("ALERT: MimeType not supported. ");
      printf("Player supports only audio 0 RTP/AVP 0 or other_media_type.\n");
      return (0);
    }
    if (!get_int(p, sl, "audio 0 RTP/AVP", " \t",
		 (int *) &curr_payload_type, 0)) {
      printf("ALERT: Missing paylod type from Media Stream description.\n");
      return (0);
    }
    switch (curr_payload_type) {
    case (RTP_PAYLOAD_PCMU):	/* u-law */
      s->max_bit_rate = 64000;
      s->max_pkt_size = 400;
      break;
    case (RTP_PAYLOAD_PCMA):	/* a-law */
      s->max_bit_rate = 64000;
      s->max_pkt_size = 400;
      break;
    case (RTP_PAYLOAD_L16_2):	/* linear 16, 44.1khz, 2 channel */
      s->max_bit_rate = 705600;
      s->max_pkt_size = 4410;
      break;
    case (RTP_PAYLOAD_L16_1):	/* linear 16, 44.1khz, 1 channel */
      s->max_bit_rate = 705600;
      s->max_pkt_size = 4410;
      break;
    case (RTP_PAYLOAD_RTSP):	/* 101, Dynamic. */
      if (!get_int(p, sl, "MaxBitRate", ":\t", 0, &s->max_bit_rate)) {
	printf("ALERT: MaxBitRate not found in descriptor.");
	return (0);
      }
      if (!get_int(p, sl, "MaxPktSize", ":\t", 0, &s->max_pkt_size)) {
	printf("ALERT: MaxBitRate not found in descriptor.");
	return (0);
      }
      break;
    }
  }

  if (s->mime_type == MIME_AUDIO) {
	switch (curr_payload_type) {
	case (RTP_PAYLOAD_PCMU):	/* u-law */
		PCM_hdr.wFormatTag = 7;
		PCM_hdr.nChannels = 1;
		PCM_hdr.nSamplesPerSec = 8000;
		PCM_hdr.wBitsPerSample = 8;
		PCM_hdr.nBlockAlign = 1; 
		PCM_hdr.nAvgBytesPerSec = 8000;   
      break;
    case (RTP_PAYLOAD_PCMA):	/* a-law */
		PCM_hdr.wFormatTag = 6;
		PCM_hdr.nChannels = 1;
		PCM_hdr.nSamplesPerSec = 8000;
		PCM_hdr.wBitsPerSample = 8;
		PCM_hdr.nBlockAlign = 1; 
		PCM_hdr.nAvgBytesPerSec = 8000;    
      break;
    case (RTP_PAYLOAD_L16_2):	/* linear 16, 44.1khz, 2 channel */
		PCM_hdr.wFormatTag = 1;
		PCM_hdr.nChannels = 2;
		PCM_hdr.nSamplesPerSec = 44100;
		PCM_hdr.wBitsPerSample = 16;
		PCM_hdr.nBlockAlign = 4; 
		PCM_hdr.nAvgBytesPerSec = 176400;    
      break;
    case (RTP_PAYLOAD_L16_1):	/* linear 16, 44.1khz, 1 channel */
		PCM_hdr.wFormatTag = 1;
		PCM_hdr.nChannels = 1;
		PCM_hdr.nSamplesPerSec = 44100;
		PCM_hdr.wBitsPerSample = 16;
		PCM_hdr.nBlockAlign = 2; 
		PCM_hdr.nAvgBytesPerSec = 88200;    
	break;
    case (RTP_PAYLOAD_RTSP):	/* 101, Dynamic. */
			if (DescriptionFormat == DESCRIPTION_MH_FORMAT) {
			  if ((n = get_string(p, sl, "TypeSpecificData", " =\t")) == 0) {
			printf("ALERT: Missing TypeSpecificData needed to set audio "
				   "parameters.\n");
			return (0);
			  }
			} else {
			  if ((n = get_string(p, sl, "a=TypeSpecificData", ":\t")) == 0) {
			printf("ALERT: Missing TypeSpecificData needed to set audio "
				   "parameters.\n");
			return (0);
			  }
			}

			if (*n != '"') {
			  printf("ALERT: Missing starting quote character from TypeSpecificData"
				 " string.\n");
			  return (0);
			}

			n++;
			if (!(e = strpbrk(n, "=> \"\r\n\t"))) {
			  printf("ALERT: Missing terminator character to Base64 encoded "
				 "string.\n");
			  return (0);
			}
			/* decode the PCM header */
			assert((e - n) == (sizeof(PCM_HDR) * 4 / 3));
			if (!decode_base64((u_char *) & pcm_header, n, sizeof(PCM_HDR))) {
			  printf("ALERT: Encountered an invalid Base64 character in encoded "
				 "string.\n");
			  return (0);
			}
			/* the server will put the pcm_header in NBO order prior to encoding. */
			if (!big_endian()) {
			  swap_word(&pcm_header.usFormatTag, 1);
			  swap_word(&pcm_header.usChannels, 1);
			  swap_dword(&pcm_header.ulSamplesPerSec, 1);
			  swap_word(&pcm_header.usBitsPerSample, 1);
			  swap_word(&pcm_header.usSampleEndianness, 1);
			}
			assert(pcm_header.usChannels != 0);
			/* Setup WAVEFORMATEX structure to be used by CWaveOutput::ParseWavData */
			PCM_hdr.wFormatTag = pcm_header.usFormatTag;
			PCM_hdr.nChannels = pcm_header.usChannels;
			PCM_hdr.nSamplesPerSec = pcm_header.ulSamplesPerSec;
			PCM_hdr.wBitsPerSample = pcm_header.usBitsPerSample;

			assert(PCM_hdr.nChannels != 0);
			PCM_hdr.nBlockAlign = (PCM_hdr.nChannels * PCM_hdr.wBitsPerSample) / 8;
			PCM_hdr.nAvgBytesPerSec = PCM_hdr.nSamplesPerSec * PCM_hdr.nBlockAlign;
      break;
    }
	PCM_hdr.CbSize = 0;
	s->PCM_hdr.usFormatTag = PCM_hdr.wFormatTag;
	s->PCM_hdr.usChannels = PCM_hdr.nChannels;
	s->PCM_hdr.ulSamplesPerSec = PCM_hdr.nSamplesPerSec;
	s->PCM_hdr.usBitsPerSample = PCM_hdr.wBitsPerSample;
    s->PCM_hdr.usSampleEndianness = 1;
  }    
  return (sdl);
}

/* Here, we deal with media announcements of each stream. */
int
set_stream_settings(char *b, int len, struct SESSION_STATE * state)
{
  STREAM         *s;		/* stream descriptor pointer */
  char           *p, *pp;	/* buffer pointer */
  int             sl;		/* search limit */
  int             sdl;		/* stream descriptor length */
  int             i;
  int             num_streams;

  /* return 0 if invalid or incomplete settings. */

  if (DescriptionFormat == DESCRIPTION_MH_FORMAT) {
    if ((p = get_int(b, len, "Duration", " =\t", 0,
		     &state->settings.duration)) == 0) {
      printf("ALERT: Missing Duration from Media Container.\n");
      return (0);
    }
    if ((p = get_int(b, len, "NumStreams", " =\t", &num_streams, 0)) == 0) {
      printf("ALERT: Missing NumStreams from Media Container.\n");
      return (0);
    }
    sl = len - (int) (p - b);	/* adjust search length for new reference. */
  } else {
    num_streams = 0;
    for (pp = b, sl = len; (p = get_string(pp, sl, "m=", ""));) {
      sl -= (int) (p - pp);
      num_streams++;
      pp = p;
    }
  }

  if (num_streams == 0) {
    printf("ALERT: Missing NumStreams from Media Container.\n");
    return (0);
  }
  state->settings.num_streams = num_streams;

  /* locate the beginning of the MediaStreams description section. */
  if (DescriptionFormat == DESCRIPTION_MH_FORMAT) {
    if ((p = get_string(p, sl, "<MediaStreams>", " \t\r\n")) == 0) {
      printf("ALERT: Missing <MediaStreams> section description.\n");
      return (0);
    }
  } else {
    if ((p = get_string(b, len, "m=", "")) == 0) {
      printf("ALERT: Missing Media Announcement section description.\n");
      return (0);
    }
    p -= 2;
  }

  sl = len - (int) (p - b);	/* adjust search length for new reference. */

  for (i = 0; i < num_streams; i++, p += sdl) {
    /*
     * allocate stream descriptor.  Stream descriptors are freed by
     * free_streams(). */
    s = (void *) calloc(1, sizeof(STREAM));
    s->next = state->streams;
    state->streams = s;		/* add new descriptor to head of list. */

    /* collect & set one set of stream properties */
    sdl = set_one_stream(p, sl, s);
    sl -= sdl;
    if (!sdl || (sl < 0))
      return (0);
  }

  /* enough for now. */
  return (1);
}



/******************************************
 *         send message functions
 ******************************************/
void 
send_hello_request( const char *sUrl)
{
   char b [256];
   if (sUrl == NULL) 
      sprintf( b, "%s * %s %d\n", HELLO_TKN, 
			RTSP_VER, RTSP_send_seq_num++ );
   else 
      sprintf( b, "%s %s %s %d\n", \
         HELLO_TKN, sUrl, RTSP_VER, RTSP_send_seq_num++ );
   strcat(b, "User-Agent: LSSP ");


  strcat(b, "\r\n\r\n");

   bwrite( b, (u_short) strlen( b ) );
      RTSP_last_request = RTSP_HELLO_METHOD;
}

void
send_close_request(void)
{
  char            b[256];

  sprintf(b, "%s * %s %d\r\n\r\n", CLOSE_TKN, RTSP_VER, RTSP_send_seq_num++);
  bwrite(b, (u_short) strlen(b));
  RTSP_last_request = RTSP_CLOSE_METHOD;
  authenticated = 0;
}

void
send_get_request(const char *sUrl)
{
  char            b[256];

  /* save the url string for future use in setup request. */
  strcpy(p_state.url, p_state.serverurl);
  strcat(p_state.url, sUrl);
  sprintf(b, "%s %s %s %d\n", GET_TKN, p_state.url, 
		RTSP_VER, RTSP_send_seq_num++);
  strcat(b, "Accept: application/sdp; application/x-rtsp-mh\n");
  strcat(b, "\r\n\r\n");
  bwrite(b, (u_short) strlen(b));
  RTSP_last_request = RTSP_GET_METHOD;
}

void
send_setup_request(void)
{
  char            b[256];

  sprintf(b, "%s %s %s %d\n", SETUP_TKN, p_state.url, RTSP_VER,
	  RTSP_send_seq_num++);
  assign_udpfd(udp_open());
  p_state.settings.data_port = udp_getport(udp_fd, &p_state.settings.saddr_in);
  sprintf(b + strlen(b), "Transport: rtp/udp;port=%d\r\n\r\n",
	  p_state.settings.data_port);

  /*  printf("Send setup message: \n%s \n", b);*/
  bwrite(b, (u_short) strlen(b));
  RTSP_last_request = RTSP_SETUP_METHOD;
}

void
send_play_range_request(char *range)
{
  char            b[256];

  sprintf(b, "%s %s %s %d\n", PLAY_TKN, p_state.url, RTSP_VER,
	  RTSP_send_seq_num++);
  /* Add the session#, this is a dummy version. */
  if (p_state.flags & SS_USE_SESSION_ID)
    add_session_id(b, &p_state);

  if (range && *range)
    sprintf(b + strlen(b), "range: %s\n", range);
  else if (p_state.flags & SS_PLAY_RANGE)
    add_play_range(b, &p_state);
  else if (end_of_streams(p_state.streams))	/* start from beginning
						 * again. */
    sprintf(b + strlen(b), "Range: smpte=00:00:00-\n");
  strcat(b, "\r\n\r\n");

  bwrite(b, (u_short) strlen(b));
  RTSP_last_request = RTSP_PLAY_METHOD;
}

void
send_pause_request(void)
{
  char            b[256];

  sprintf(b, "%s %s %s %d\r\n\r\n", PAUSE_TKN, p_state.url, RTSP_VER,
	  RTSP_send_seq_num++);
  bwrite(b, (u_short) strlen(b));
  RTSP_last_request = RTSP_PAUSE_METHOD;
}


/******************************************
 *   received message handler functions
 ******************************************/

int
handle_PEP(char *buf, int StrLen)
{
	int PEP_Start;
	int PEP_End = 0;
	int ParenCount = 1;
	char *TempStr;
	char cc;

	if ((TempStr = strstr(buf, "PEP:")) == NULL)
		if ((TempStr = strstr(buf, "PEP-Info:")) == NULL)
			if ((TempStr = strstr(buf, "C-PEP:")) == NULL)
				TempStr = strstr(buf, "C-PEP-Info:");
	if (TempStr == NULL) 
		return 0;
	PEP_Start = TempStr - buf;
	TempStr = strstr(buf + PEP_Start, "{");
	if (TempStr == NULL) 
		return -2;
	
	PEP_End = TempStr - buf;
	do {
		PEP_End++;
		if (PEP_End > StrLen)
			return -2;
		cc = *(++TempStr);
		ParenCount+= (cc == '{') - (cc == '}');
	} while (ParenCount);

	if (strstr(buf + PEP_Start, "{strength must}") == NULL)
		return 0;
	else 
		return -1;
}

void
handle_hello_request(void)
{
  interface_hello(OpenFileName);
  discard_msg();		/* remove remainder of message from in_buffer */
  /* here's were you can add authentication checks and rejection */

  send_reply(200, 0);		/* OK */
}

void
handle_hello_reply(int status)
{
  discard_msg();		/* remove remainder of message from in_buffer */

  if (status < 203)
    authenticated = 1;

  /*if (OpenFileName != NULL) 
	  send_get_request(OpenFileName);*/
}


int
handle_get_reply(int status)
{
  char           *p;
  char            b[64];
  int             n;
  int             hl;		/* message header length */
  int             bl;		/* message body length */

  /* return !0 if get response is dealt with and setup should be sent. */
  if (status > 202) {
    discard_msg();		/* remove remainder of message from in_buffer */
    return (0);
  }
  /* free up stream descriptors from any previous get. */
  free_streams(&p_state);

  get_msg_len(&hl, &bl);	/* set header and message body length */

  /* get the content type */
  if ((p = get_string(in_buffer, hl, (char *) sContentType, " :")) == 0) {
    printf("ALERT: required Content-Type header field is missing.\n");
    remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
    return (0);
  }
  if ((n = sscanf(p, " %60s ", b)) != 1) {
    printf("ALERT: Content-Type value string not present\n");
    remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
    return (0);
  }

   if (handle_PEP(in_buffer, hl) == -1) { 
    remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
	send_reply(420, 0);
    return (0);
  }

  if (strcasecmp(b, "application/sdp")) {
    if (strcasecmp(b, "application/x-rtsp-mh")) {
      printf("ALERT: Content-Type (%s) not recognized.\n", b);
      remove_msg(hl + bl);
      /* remove remainder of message from in_buffer */
      return (0);
    }
    DescriptionFormat = DESCRIPTION_MH_FORMAT;
  } else
    DescriptionFormat = DESCRIPTION_SDP_FORMAT;

  /* get the media announcements for each stream. */
  if (!set_stream_settings(&in_buffer[hl], bl, &p_state)) {
    remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
    return (0);
  }
  remove_msg(hl + bl);		/* remove remainder of message from in_buffer */

  return (1);
}

void
handle_setup_reply(int status)
{
  int             hl;		/* message header length */
  int             bl;		/* message body length */
  int             val;

  if (status < 203) {
    p_state.settings.session_id = 0;
    interface_new_session(0);
    get_msg_len(&hl, &bl);	/* set header and message body length */
    if (get_int(in_buffer, hl, (char *) sSession, " :", &val, 0)) {
      p_state.flags |= SS_USE_SESSION_ID;
      val = 0; /* This session is diffrent from the session in the reply. */
      /* Dummy version of session_id. */
      p_state.settings.session_id = val;
    } else
      p_state.flags &= ~SS_USE_SESSION_ID;
  }
  if (handle_PEP(in_buffer, hl) == -1) 
	send_reply(420, 0);
  remove_msg(hl + bl);
}

void
handle_play_reply(int status)
{
  int             hl;		/* message header length */
  int             bl;		/* message body length */
  long            l;

  get_msg_len(&hl, &bl);

  if (status < 203) {
    if (get_int(in_buffer, hl, (char *) sSession, ": ", 0, &l)) {
      if (!(p_state.flags & SS_USE_SESSION_ID)) {
	p_state.flags |= SS_USE_SESSION_ID;
	p_state.settings.session_id = l;
      } else if ((long) p_state.settings.session_id != l)
	printf("Invalid Session ID returned by server.");
    }
	if (handle_PEP(in_buffer, hl) == -1) { 
		remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
		send_reply(420, "Bad Extension\n");
		return;
    }

    interface_start_play(p_state.settings.session_id);
  }
  remove_msg(hl + bl);
}

void
handle_pause_reply(int status)
{
  discard_msg();		/* remove remainder of message from in_buffer */
}

void
handle_close_reply(int status)
{
  discard_msg();		/* remove remainder of message from in_buffer */
  if (status < 203) {
    free_streams(&p_state);
    clear_udpfd();		/* close down udp socket connection. */
    interface_close_stream(p_state.settings.session_id);
    /* shut down the connection but leave application running. */
    tcp_close(main_fd);
    main_fd = 0;
  }
}


/**********************************************************
 *    server socket connection and receive data handling
 *********************************************************/

int
server_connect(const char *server, u_short port, int *sock)
{
  int             got_host = 0;
  struct sockaddr_in addr;
  struct hostent *h = gethostbyname(server);
  if (h)
    got_host = 1;
  else {
    /* dot notation? */
    u_int32         net_addr = inet_addr(server);
    h = gethostbyaddr((const char *) &net_addr, sizeof(u_int32), PF_INET);
    if (h)
      got_host = 1;
  }
  if (got_host) {
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *(u_int32 *) h->h_addr;
    addr.sin_port = htons(port);
    *sock = tcp_open((struct sockaddr *) & addr, sizeof(addr));
    if (*sock < 0)
      got_host = 0;
  }
  return got_host;
}

void
udp_data_recv(RTSP_SOCK fd)
{
  char            buffer[16384];
  int             n, l;
  RTP_HDR        *rtp;
  RTP_OP         *op;
  STREAM         *s;
  short           data_len;
  char           *data;
  int             addlen;
  u_short         seq_no;
  u_short         sflags;
  u_int32         oflags;

  n = recv(fd, buffer, sizeof(buffer), 0);
  if (n < 0)
    return;

  /* ignore data if it is not long enough to contain at least the RTP header */
  if (n < (int) sizeof(RTP_HDR)) {
    printf("ALERT: UDP data received did not include RTP header.\n");
    return;
  }
  rtp = (RTP_HDR *) buffer;

  if (rtp->payload != RTP_PAYLOAD_RTSP &&
      rtp->payload != RTP_PAYLOAD_L16_2 &&
      rtp->payload != RTP_PAYLOAD_L16_1 &&
	  rtp->payload != RTP_PAYLOAD_PCMA &&
      rtp->payload != RTP_PAYLOAD_PCMU) {
    printf("ALERT: Unsupported payload type: %d.\n",
	   rtp->payload);
    return;
  }
  /* reject if specific fields are not correct. */
  assert(p_state.streams);

  for (s = p_state.streams; s && (s->stream_id != rtp->ssrc); s = s->next);
  if (!s) {
    printf("ALERT: RTP header rejected because SSRC not equal to stream id.\n");
    return;
  }
  seq_no = ntohs(rtp->seq_no);
  s->seq_num = seq_no;		/* set this streams new sequence number */

  /* add code hear to use time stamp. */

  if (rtp->csrc_len != 0) {
    printf("ALERT: RTP header rejected because CSRC length not 0.\n");
    return;
  }
  if (rtp->extension == 1) {
    addlen = sizeof(RTP_OP);
    op = (RTP_OP *) (buffer + sizeof(RTP_HDR));
    l = ntohs(op->op_code_data_length);
    if (ntohs(op->op_code) != RTP_OP_PACKETFLAGS)
      printf("ALERT: RTP extension header op_code invalid.\n");
    if (l == RTP_OP_CODE_DATA_LENGTH) {
      sflags = s->stream_flags;
      oflags = ntohl(op->op_code_data[0]);
      if (oflags & RTP_FLAG_LASTPACKET) {
	sflags |= STREAM_LAST_PACKET;
	/* if last packet and still in PLAY state return to READY state */
	if (p_state.cur_state == PLAY_STATE) {
	  p_state.cur_state = READY_STATE;
	  interface_stream_done(p_state.settings.session_id);
	}
      } else
	sflags &= ~STREAM_LAST_PACKET;

      if (oflags & RTP_FLAG_KEYFRAME)
	sflags |= STREAM_KEY_FRAME;
      else
	sflags &= ~STREAM_KEY_FRAME;
      s->stream_flags = sflags;
    } else
      printf("ALERT: RTP extension header op_code_data_length invalid.\n");
    addlen += (l * 4) - 4;
  } else {
    addlen = 0;
  }

  data = buffer + sizeof(RTP_HDR) + addlen;
  data_len = n - (sizeof(RTP_HDR) + addlen);

  if (debug_toggle) {
    printf("RTP_HEADER: ");
    dump_buffer(buffer, sizeof(RTP_HDR) + addlen + 2);
  }
  if (data_len) {
    if (PCM_hdr.wBitsPerSample == 16 && !big_endian()) {
      swap_word((unsigned short *) data, (data_len / 2));
    }
    interface_stream_output(p_state.settings.session_id, data_len, data);

  }
  return;
}


/***************************************
 *   player command handling routines
 **************************************/

void
handle_command(const char *c)
{
  char            cmd[256];
  char           *tok;
  char            ctok;
  static int      seen_open = 0;
  char            msg[512];

  printf("\nhandle_command: %s\n", c);
  strncpy(cmd, c, sizeof(cmd));
  tok = strtok(cmd, " \t\n");
  if (!tok) {
    interface_error("Command not found.\n");
    return;
  }
  ctok = tolower(*tok);
  if (ctok == 'o') {		/* open */
    char            url[1024];
    char            file_name[256];
    char            server_name[256];
    u_short         port;
    RTSP_SOCK       sock;
      char file_name2[256];
      char server_name2[256];
      u_short port2;
      int using_a_proxy = 0;

    if (seen_open) {
      interface_error("\nalready connected\n");
      return;
    }
    tok = strtok(NULL, " \t\n");
    if (!tok) {
      interface_error("\nOPEN requires a URL in the form "
		      "'rtsp://server[:port][/file]'\n");
      return;
    }
    strcpy(url, tok);
    if (parse_url(url, server_name, &port, file_name)) {
         if (strlen(file_name) > 0) 
         {
            if (parse_url(file_name, server_name2, &port2, file_name2) &&
               (strlen(server_name2) != 0)) 
            {
               strcpy(p_state.serverurl, "rtsp://");
               strcat(p_state.serverurl, server_name2);
               strcat(p_state.serverurl, "/"); 
               /* port2 and file_name2 are discarded. sorry. */
               using_a_proxy = 1;
            }
         }
         if (!using_a_proxy) 
         {
	    strcpy(p_state.serverurl, "rtsp://");
	    strcat(p_state.serverurl, server_name);
            strcat(p_state.serverurl, "/");
         } 
         if(server_connect(server_name, port, &sock))
         {
            if (using_a_proxy)
               handle_event( CMD_OPEN, 0, file_name );
            else /* Not using a proxy */
               handle_event( CMD_OPEN, 0, 0);

		strcpy(OpenFileName, file_name);
		seen_open = 1;
			main_fd = sock;
      } else {
	sprintf(msg, "\nunable to connect to server //%s:%d\n",
		server_name, port);
	interface_error(msg);
      }
    } else
      interface_error("\nunable to parse url\n");
  } else if (ctok == 'g') {	/* get */
    char            url[1024];
    char            file_name[256];
    char            server_name[256];
    u_short         port;

    tok = strtok(NULL, " \t\n");
    if (!tok) {
      sprintf(msg, "\nGET requires an URL in the form "
	      "'path/mediafile.wav'\n");
      interface_error(msg);
      return;
    }
    strcpy(url, tok);
    if (parse_url(url, server_name, &port, file_name))
      handle_event(CMD_GET, 0, file_name);
    else
      interface_error("\nunable to parse url\n");
  } else if (ctok == 'p') {	/* play */
    char            range[64];

    tok = strtok(NULL, " \t\n");
    *range = '\0';
    if (tok) {
      if (strchr(tok, '=')) {
	strncpy(range, tok, 63);
	range[63] = '\0';
      } else {			/* range doesn't have smpte prefix */
	strcpy(range, "smpte=");
	strcat(range, tok);
      }
    }
    handle_event(CMD_PLAY, 0, range);
  } else if (ctok == 'z') {	/* pause */
    handle_event(CMD_PAUSE, 0, 0);
  } else if (ctok == 'c') {	/* close */
    handle_event(CMD_CLOSE, 0, 0);
  } else if (ctok == 'q' || ctok == 'x') {	/* quit, exit */
    Quit = 1;			/* this will disable the timer event handler */
    terminate(0);
  } else {
    interface_error("\nunrecognized command\n");
  }
}


/***************************************
 *       player state machine
 **************************************/

void
handle_event(int event, int status, char *buf)
{
  static const char sStateErrMsg[] = "State error: event %d in state %d\n";
  char            msg[2048];

  switch (p_state.cur_state) {
  case INIT_STATE:
    {
      switch (event) {
      case RTSP_GET_RESPONSE:
	if (handle_get_reply(status))
	  send_setup_request();	/* now send a setup request */
	break;

      case RTSP_SETUP_RESPONSE:
	handle_setup_reply(status);
	if (status < 203) {
	  p_state.cur_state = READY_STATE;
	  handle_event(CMD_PLAY, 0, 0);
	}
	break;

      case RTSP_CLOSE_RESPONSE:
	handle_close_reply(status);
	break;

      case RTSP_HELLO_RESPONSE:
	handle_hello_reply(status);
	break;

      case RTSP_HELLO_METHOD:
	handle_hello_request();
	break;

      case CMD_OPEN:
	send_hello_request(buf);
	break;

      case CMD_GET:
	send_get_request(buf);
	break;

      case CMD_CLOSE:
	send_close_request();
	break;

      case RTSP_SESSION_METHOD:/* method not valid this state. */
	send_reply(455, "Accept: HELLO\n");
	break;

      default:
	sprintf(msg, sStateErrMsg, event, p_state.cur_state);
	interface_error(msg);
	if (event < CMD_OPEN) {
	  discard_msg();	/* remove remainder of message from in_buffer */
	  if (event < RTSP_SETUP_RESPONSE)	/* method not implemented */
	    send_reply(501, "Accept: HELLO\n");
	}
	break;
      }
      break;
    }				/* INIT state */

  case READY_STATE:
    {
      switch (event) {
      case RTSP_PLAY_RESPONSE:
	handle_play_reply(status);
	if (status < 203)
	  p_state.cur_state = PLAY_STATE;
	break;

      case RTSP_GET_RESPONSE:
	if (handle_get_reply(status)) 
	  send_setup_request();	
	else 
		p_state.cur_state = INIT_STATE;
	break;
		
      case RTSP_SETUP_RESPONSE:
	handle_setup_reply(status);
	if (status < 203) 
	  handle_event(CMD_PLAY, 0, 0);
	else
		 p_state.cur_state = INIT_STATE;
	break;

      case RTSP_CLOSE_RESPONSE:
	handle_close_reply(status);
	if (status < 203)
	  p_state.cur_state = INIT_STATE;
	break;

      case RTSP_HELLO_RESPONSE:
	handle_hello_reply(status);
	break;

      case RTSP_HELLO_METHOD:
	handle_hello_request();
	break;

      case CMD_PLAY:
	send_play_range_request(buf);
	break;

      case CMD_GET:		/* allow get w/o changing state. */
	send_get_request(buf);
	break;

      case CMD_CLOSE:
	send_close_request();
	break;

      default:
	sprintf(msg, sStateErrMsg, event, p_state.cur_state);
	interface_error(msg);
	if (event < CMD_OPEN) {
	  discard_msg();	/* remove remainder of message from in_buffer */
	  if (event < RTSP_SETUP_RESPONSE)	/* method not implemented */
	    send_reply(501, "Accept: HELLO\n");
	}
	break;
      }
      break;
    }				/* READY state */

  case PLAY_STATE:
    {
      switch (event) {
      case RTSP_PLAY_RESPONSE:
	handle_play_reply(status);
	if (status >= 203)
	  p_state.cur_state = READY_STATE;
	break;

      case RTSP_PAUSE_RESPONSE:
	handle_pause_reply(status);
	if (status < 203)
	  p_state.cur_state = READY_STATE;
	break;

      case RTSP_CLOSE_RESPONSE:
	handle_close_reply(status);
	if (status < 203)
	  p_state.cur_state = INIT_STATE;
	break;

      case RTSP_HELLO_RESPONSE:
	handle_hello_reply(status);
	break;

	case RTSP_GET_RESPONSE:
	if (handle_get_reply(status)) 
	  send_setup_request();	
	else 
		p_state.cur_state = INIT_STATE;
	break;
		
      case RTSP_SETUP_RESPONSE:
	handle_setup_reply(status);
	if (status < 203) 
	  handle_event(CMD_PLAY, 0, 0);
	else
		 p_state.cur_state = INIT_STATE;
	break;

      case RTSP_HELLO_METHOD:
	handle_hello_request();
	break;

      case CMD_PLAY:
	send_play_range_request(buf);
	break;

      case CMD_PAUSE:
	send_pause_request();
	break;

      case CMD_GET:		/* allow get w/o changing state. */
	send_get_request(buf);
	break;

      case CMD_CLOSE:
	send_close_request();
	break;

      default:
	sprintf(msg, sStateErrMsg, event, p_state.cur_state);
	interface_error(msg);
	if (event < CMD_OPEN) {
	  discard_msg();	/* remove remainder of message from in_buffer */
	  if (event < RTSP_SETUP_RESPONSE)	/* method not implemented */
	    send_reply(501, "Accept: HELLO\n");
	}
	break;
      }
      break;
    }				/* PLAY state */

  default:			/* invalid/unexpected current state. */
    {
      sprintf(msg, "State error: unknown state=%d, event code=%d\n",
	      p_state.cur_state, event);
      interface_error(msg);
      if (event < CMD_OPEN)
	discard_msg();		/* remove remainder of message from in_buffer */
    }
    break;
  }				/* end of current state switch */
}
