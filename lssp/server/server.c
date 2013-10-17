
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include "machdefs.h"
#include "socket.h"
#include "config.h"
#include "eventloop.h"
#include "messages.h"
#include "socket.h"
#include "scheduler.h"
#include "server.h"
#include "util.h"
#include "rtsp.h"
#include "streamer.h"
#include "session.h"
#include "parse.h"
#include "rtp.h"

struct SESSION_STATE s_state;
static u_short    StreamSessionID = 0;
char server_name[256];
char objurl[256];
int authenticated = 0;
u_char curr_payload_type;
int DescriptionFormat;
int DescribedFlag = 0;

void
init_server(int argc, char **argv)
{
    int i = 0;
    schedule_init();
    init_config(argc, argv); // config.h
    printf("Control channel port set to %d.\n", get_config_port());
    printf("BasePath set to \"%s\"\n", BasePath);

    for (; i < MAXSTREAMSESSIONS; i++)
      s_state.sessionlist[i] = 0;
}


/************************************************************
 *     server specific support and message building routines
 ***********************************************************/

char
bin_to_base64(int i)
{
    static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopq"
    "rstuvwxyz0123456789+/";
    assert(i < (int) strlen(base64));
    return (base64[i]);
}

void
encode_Base64(char *o, char *i, int len)
{
    int             n;
    u_int32         v;
    u_int32         a = 0;	/* accumulated value */
    int             s = 0;	/* shift register */

    for (n = 0; n < len; n++) {
      v = (u_int32) * i++;
      a <<= 8;
      s += 8;
      a |= v;

      while (s >= 6) {
        s -= 6;
        v = (a >> s) & 0x3fL;
        *o++ = bin_to_base64(v);
      }
    }
    *o = '\0';
    return;
}

int
get_RIFF_header(int fd, STREAM * s)
{
    char            buf[128];
    int             n;
    u_long          Bps;		/* bytes per second */
    int             BpSample;	/* bytes per sample */
    u_int32         len;
    WAVEFMTX       *px;
    int             i;
    int             found;
    u_short         bps;		/* bits per sample */

    n = read(fd, buf, 128);
    if (n < (int) (0x14 + sizeof(WAVEFMTX)) || memcmp(buf, "RIFF", 4) ||
        memcmp(buf + 8, "WAVEfmt ", 8))
      return (0);
    memcpy(&len, &buf[0x10], sizeof(u_int32));

    assert(len <= sizeof(WAVEFMTX));
    px = (WAVEFMTX *) (buf + 0x14);

    /* build a PCM header in network byte order before encoding it. */
    s->PCM_hdr.usFormatTag = htons(px->wFormatTag);
    s->PCM_hdr.usChannels = htons(px->nChannels);
    s->PCM_hdr.ulSamplesPerSec = htonl(px->nSamplesPerSec);
    if (len < 0x12)		/* older format wave file (WAVEFORMAT). */
      bps = (u_int16) (px->nChannels * (px->nBlockAlign * 8));
    else				/* newer format (WAVEFORMATX) */
      bps = px->wBitsPerSample;
    s->PCM_hdr.usBitsPerSample = htons(bps);

    s->PCM_hdr.usSampleEndianness = htons(1);	/* big endian */

    /* now Base64 encode the PCM header data */
    assert(sizeof(s->type_data) > (sizeof(PCM_HDR) * 4 / 3));
    encode_Base64((char *) s->type_data, (char *) &s->PCM_hdr,
		sizeof(PCM_HDR));

    /* locate the data. */
    for (i = 0x14 + len, found = 0; i < (n - 3); i++) {
      if (!memcmp(&buf[i], "data", 4)) {
        found = 1;
        break;
      }
    }

    if (!found)
      return (0);

    s->mime_type = MIME_AUDIO;
    s->stream_fd = fd;
    memcpy(&s->byte_count, &buf[i + 4], sizeof(u_int32));

    /* position at start of data */
    s->start_offset = i + 8;

    BpSample = bps / 8;		/* bytes per sample */
    if (bps % 8)
      BpSample++;
    Bps = BpSample * px->nSamplesPerSec;	/* bytes/sec. */
    s->bytes_per_sec = Bps;

    s->duration = (s->byte_count * 1000) / Bps;

    /* set MaxBitRate, AvgBitRate, MaxPktSize, AvgPktSize, Preroll. */
    s->preroll = 0;
    s->max_bit_rate = s->ave_bit_rate = Bps * 8;
    s->xmit_interval = 50;	/* fixed at 50 milliseconds */

    /* set packet size = bytes/sec / (packets/sec) */
    s->max_pkt_size = s->ave_pkt_size = Bps / (1000 / s->xmit_interval);

    curr_payload_type = PCM2RTP_payload_type(&s->PCM_hdr);

    return (1);
}

void
add_stream(char *r, STREAM * s)
{
    char            media[100];

    if (DescriptionFormat == DESCRIPTION_MH_FORMAT) {
      strcat(r, "<MediaStream\n");
      sprintf(r + strlen(r), "StreamID = %d\n", s->stream_id);
      if (s->mime_type == MIME_AUDIO)
        strcpy(media, "audio/x-pn-wav");
      else
        strcpy(media, "plain/text");
      sprintf(r + strlen(r), "MimeType = %s\n", media);
      sprintf(r + strlen(r), "MaxBitRate = %ld\n", s->max_bit_rate);
      sprintf(r + strlen(r), "AvgBitRate = %ld\n", s->ave_bit_rate);
      sprintf(r + strlen(r), "MaxPktSize = %ld\n", s->max_pkt_size);
      sprintf(r + strlen(r), "AvgPktSize = %ld\n", s->ave_pkt_size);
      sprintf(r + strlen(r), "Preroll = %d\n", s->preroll);
      sprintf(r + strlen(r), "Duration = %ld\n", s->duration);
      sprintf(r + strlen(r), "StartTime = %ld\n", s->start_time);
    } else {
      /* SDP format. */
      if (s->mime_type == MIME_AUDIO) {
        strcpy(media, "audio 0 RTP/AVP ");
        sprintf(media + strlen(media), "%d", (int) curr_payload_type);
      } else
        strcpy(media, "other_media_type");

      sprintf(r + strlen(r), "m=%s\n", media);
      /*sprintf(r + strlen(r), "a=StreamID:%d\n", s->stream_id);*/

      if (curr_payload_type == RTP_PAYLOAD_RTSP) {
        /* PN Dynamic type (payload = 101) */
        sprintf(r + strlen(r), "a=MaxBitRate:%ld\n", s->max_bit_rate);
        sprintf(r + strlen(r), "a=MaxPktSize:%ld\n", s->max_pkt_size);
      }
    }

    if ((*s->type_data) && (curr_payload_type == RTP_PAYLOAD_RTSP)) {
      if (DescriptionFormat == DESCRIPTION_SDP_FORMAT)
        sprintf(r + strlen(r), "a=TypeSpecificData:\"%s\"\n",
	        s->type_data);
      else
        sprintf(r + strlen(r), "TypeSpecificData = \"%s\"\n",
	        s->type_data);
    }
}

char             *
get_SDP_user_name()
{
    return "- ";
}

char             *
get_SDP_session_id()
{
    return "2890844256 ";
}

char             *
get_SDP_version()
{
    return "2890842807 ";
}

/* Return the IP address of this server. */
char             *
get_address()
{
    static char     Ip[256];
    u_char          addr1, addr2, addr3, addr4, temp;
    u_long          InAddr;
    struct hostent *host = gethostbyname(server_name);

    temp = 0;
    InAddr = *(u_int32 *) host->h_addr;
    addr4 = (u_char) ((InAddr & 0xFF000000) >> 0x18);
    addr3 = (u_char) ((InAddr & 0x00FF0000) >> 0x10);
    addr2 = (u_char) ((InAddr & 0x0000FF00) >> 0x8);
    addr1 = (u_char) (InAddr & 0x000000FF);

    sprintf(Ip, "%d.%d.%d.%d", addr1, addr2, addr3, addr4);

    return Ip;
}

int
build_get_reply(char *r, struct SESSION_STATE * s)
{
    STREAM         *stream;

    if (DescriptionFormat == DESCRIPTION_SDP_FORMAT) {
      strcpy(r, "v=0\n");

      /* Add the "Origin" field. */
      strcat(r, "o=");
      strcat(r, get_SDP_user_name());
      strcat(r, get_SDP_session_id());
      strcat(r, get_SDP_version());
      strcat(r, "IN ");		/* Network type: Internet. */
      strcat(r, "IP4 ");		/* Address type: IP4. */
      strcat(r, get_address());
      strcat(r, "\n");
      strcat(r, "s=RTSP Session\n");
      strcat(r, "i=LSSP\n");
      sprintf(r + strlen(r), "u=%s\n", objurl);


      strcat(r, "t=0 0\n");
    } else {
      /* For rtsp-mh description format. */
      strcpy(r, "<MediaDescription>\n<MediaContainer\n");
      strcat(r, "Author = \"(null)\"\n");
      strcat(r, "Title = \"(null)\"\n");
      strcat(r, "Copyright = \"(null)\"\n");
      strcat(r, "Comments = \"(null)\"\n");
      sprintf(r + strlen(r), "MaxBitRate = %ld\n", s->settings.max_bit_rate);
      sprintf(r + strlen(r), "AvgBitRate = %ld\n", s->settings.ave_bit_rate);
      sprintf(r + strlen(r), "MaxPktSize = %ld\n", s->settings.max_pkt_size);
      sprintf(r + strlen(r), "AvgPktSize = %ld\n", s->settings.ave_pkt_size);
      sprintf(r + strlen(r), "Preroll = %d\n", s->settings.preroll);
      sprintf(r + strlen(r), "Duration = %ld\n", s->settings.duration);
      sprintf(r + strlen(r), "Flags = %d\n", s->settings.media_flags);
      sprintf(r + strlen(r), "NumStreams = %d\n", s->settings.num_streams);
      strcat(r, ">\n<MediaStreams>\n");
    }

    /* Add media announcements for each stream here. */
    for (stream = s->streams; stream; stream = stream->next)
      add_stream(r, stream);

    if (DescriptionFormat == DESCRIPTION_MH_FORMAT)
      strcat(r, "</MediaStreams>\n</MediaDescription>\n");

    /*  strcat(r, "\n");*/

    return (strlen(r));

}

char             *
alloc_path_name(char *base_path, char *file_path)
{
    char           *PathName;
    char           *p;
    int             len;

    len = strlen(base_path);
    PathName = malloc(len + strlen(file_path) + 2);
    assert(PathName);
    p = base_path + len;
    if (len)
      p--;
    if ((*p == '/') && (*file_path == '/'))
      sprintf(PathName, "%s%s", base_path, file_path + 1);
    else if ((*p != '/') && (*file_path != '/'))
      sprintf(PathName, "%s/%s", base_path, file_path);
    else
      sprintf(PathName, "%s%s", base_path, file_path);

    return (PathName);
}

/******************************************
 *           send message functions
 ******************************************/

void
send_hello(void)
{
    char            b[256];

    sprintf(b, "%s * %s %d\n", HELLO_TKN, RTSP_VER, 
		RTSP_send_seq_num++);
    strcat(b, "Server: LSSP ");

    strcat(b, "\r\n\r\n");
    bwrite(b, (u_short) strlen(b));
    printf("HELLO sent.\n");
    RTSP_last_request = RTSP_HELLO_METHOD;
}

void
send_get_reply(struct SESSION_STATE * s)
{
    char           *r;		/* get reply message buffer pointer */
    char           *mb;		/* message body buffer pointer */
    int             mb_len;

    /* allocate get reply message buffer */
    assert(s->settings.num_streams);
    mb_len = 2048 * s->settings.num_streams;
    mb = malloc(mb_len);
    r = malloc(mb_len + 512);
    if (!r || !mb) {
      printf("PANIC: unable to allocate memory for DESCRIBE response.\n");
      send_reply(500, 0);		/* internal server error */
      if (r)
        free(r);
      if (mb)
        free(mb);
      return;
    }
    /* build a reply message */

    sprintf(r, "%s %d %d %s\n", RTSP_VER, 200, RTSP_recv_seq_num,
	    get_stat(200));
    add_time_stamp(r, 0);
    if (DescriptionFormat == DESCRIPTION_SDP_FORMAT)
      strcat(r, "Content-type: application/sdp\n");
    else
      strcat(r, "Content-type: application/x-rtsp-mh\n");

    /* build the message body */
    mb_len = build_get_reply(mb, s);
    sprintf(r + strlen(r), "Content-Length: %d\r\n\r\n", mb_len);
    strcat(r, mb);
    bwrite(r, (u_short) strlen(r));

    printf("Describe response message: \n");
    printf("%s \n", r);
    free(mb);
    free(r);
    printf("DESCRIBE response sent.\n");
    return;
}

void
send_setup_reply(struct SESSION_STATE * s, u_short StreamSessionID)
{
    char            b[256];

    sprintf(b, "Session: %d\n", (int) StreamSessionID);

    strcat(b, "Content-Length: 0\n");
    sprintf(b + strlen(b) , "Transport: rtp/udp;port=%d\r\n\r\n",
	    s->settings.data_port);

    printf("Setup response message: \n%s\n", b);
    send_reply(200, b);
    printf("SETUP response sent.\n");
}



/******************************************
 *     received message handler functions
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
    printf("HELLO request received.\n");
    discard_msg();		/* remove remainder of message from in_buffer */


    printf("HELLO response sent.\n");
    send_reply(200, 0);		/* OK */
    send_hello();
}

void
handle_hello_reply(int status)
{
    printf("HELLO reply received.\n");
    discard_msg();		

    if (status < 203)
      authenticated = 1;
}

int
handle_get_request(char *b)
{
    char            object[255];
    char           *p;
    char           *PathName;
    int             fd;

    u_short         port;

    DescribedFlag = 1;

    if (handle_PEP(in_buffer, strlen(in_buffer)) == -1) { 
	discard_msg();
	send_reply(420, 0);
      return (0);
    }

    printf("DESCRIBE request received.\n");
    
    if (!sscanf(b, " %*s %254s ", objurl)) {
      printf("DESCRIBE request is missing object (path/file) parameter.\n");
      discard_msg();	
      send_reply(400, 0);		/* bad request */
      return (0);
    }
    DescriptionFormat = (strstr(in_buffer, "application/sdp") != NULL);

    if (!parse_url(objurl, server_name, &port, object)) {
      printf("Mangled URL in DESCRIBE.\n");
      discard_msg();		
      send_reply(400, 0);		/* bad request */
      return (0);
    }
    discard_msg();	

    if (strstr(object, "../")) {
      printf("DESCRIBE request specified an object parameter with a path "
	     "that is not allowed. '../' not permitted in path.\n");
      /* unsupported media type */
      send_reply(403, 0);
      return (0);
    }
    p = strrchr(object, '.');
    if (!p || (strcasecmp(p, ".WAV") && strcasecmp(p, ".AU"))) {
      printf("DESCRIBE request specified an object (path/file) parameter "
	     "that is not a .WAV or .AU file.\n");
      /* unsupported media type */
      send_reply(415, "Accept: *.WAV or *.AU objects only.\n");
      return (0);
    }
    PathName = alloc_path_name(BasePath, object);
    /* open the file. */
    if ((fd = open(PathName, O_RDONLY)) == -1) {
      printf("DESCRIBE request specified an object (path/file) parameter "
	     "that could not be found.  (%s)\n", PathName);
      free(PathName);
      send_reply(404, 0);		/* not found */
      return (0);
    }
    free(PathName);

    /* add check here to see if current play state needs to be stopped. */
    free_streams(&s_state);
    /* allocate a stream descriptor */
    s_state.streams = calloc(1, sizeof(STREAM));
    s_state.settings.num_streams = 1;
    s_state.streams->stream_id = 0;
    strncpy(s_state.streams->filename, object,
	    sizeof(s_state.streams->filename));
    /* now process the RIFF file. */
    if (!get_RIFF_header(fd, s_state.streams)) {
      close(fd);
      printf("DESCRIBE method could not find a valid RIFF header in the .WAV "
	     "file.\n");
      send_reply(415, "Accept: RIFF file format only.\n");
      return (0);
    }
    s_state.settings.max_bit_rate = s_state.streams->max_bit_rate;
    s_state.settings.ave_bit_rate = s_state.streams->ave_bit_rate;
    s_state.settings.max_pkt_size = s_state.streams->max_pkt_size;
    s_state.settings.ave_pkt_size = s_state.streams->ave_pkt_size;
    s_state.settings.duration = s_state.streams->duration;

    close(fd);

    send_get_reply(&s_state);
    return (1);
}

int
handle_setup_request(char *b)
{
    char           *p;		/* buffer pointer */
    int             hl;		/* message header length */
    int             bl;		/* message body length */
    int             port;
    int             i;
    int             Stream_ID;
    STREAM         *cur_stream;

    printf("SETUP request received.\n");

    get_msg_len(&hl, &bl);	/* set header and message body length */
if (handle_PEP(in_buffer, hl) == -1) { 
      remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
	send_reply(420, 0);
      return (0);
    }
    /* only accept Transport: rtp/udp;port=1079 */
    if ((p = get_string(in_buffer, hl, "Transport", ": \t")) == 0) {
      printf("ALERT: Missing Transport settings.\n");
      remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
      send_reply(406, "Require: Transport settings of rtp/udp;port=nnnn.\n");
      return (0);
    }
    if (strncasecmp(p, "rtp/udp", 7)) {
      if (strncasecmp(p, "udp/rtp", 7)) {
        printf("ALERT: Missing Transport settings.\n");
        remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
        /* return "not acceptable" status */
        send_reply(406, "Accept: Transport settings of rtp/udp only\n");
        return (0);
      }
      printf("ALERT: please follow the spec: use rtp/udp instead of"
	     "udp/rtp.\n");
    }
    if (!get_int(p, hl, "port", "=\t", &port, 0)) {
      printf("ALERT: SETUP request missing port number setting.");
      remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
      return (0);
    }
    s_state.settings.data_port = port;

    if (DescriptionFormat == DESCRIPTION_SDP_FORMAT) {
	    Stream_ID = 0;
      cur_stream = s_state.streams;
      while (cur_stream) {
        if (cur_stream->stream_id == Stream_ID)
	break;
        cur_stream = cur_stream->next;
      }

      if (!cur_stream) {
        printf("ALERT: Stream-ID doesn't match.");
        remove_msg(hl + bl);
        return (0);
      }
      StreamSessionID++;
      if (!StreamSessionID)
        StreamSessionID++;
      for (i = 0; i < MAXSTREAMSESSIONS; i++)
        if (!s_state.sessionlist[i])
	break;

      if (i == MAXSTREAMSESSIONS) {
        printf("ALERT: Sorry, but too many sessions exists.");
        remove_msg(hl + bl);
        return (0);
      }
      s_state.sessionlist[i] = StreamSessionID;
      s_state.sessionlist_stream_ids[i] = Stream_ID;
    }
    remove_msg(hl + bl);	
    send_setup_reply(&s_state, StreamSessionID);

    return (1);
}

int
handle_play_request(char *b)
{
    STREAM         *s;
    struct STREAMER *streamer;
    char            object[256];
    int             hl;		/* message header length */
    int             bl;		/* message body length */
    int             i;
    int             found;
    int             hr, min, sec;
    long            time_index;
    char           *p;
    u_long          input_offset;
    u_long          save_offset;

    char            server_name[256];
    u_short         port;
    char            objurl[256];

    int             StreamSessionID;

    char           *PathName;
    int             fd;


    printf("PLAY request received.\n");
    get_msg_len(&hl, &bl);	/* set header and message body length */
    assert(sizeof(object) > 255);
    if (!sscanf(b, " %*s %255s ", objurl)) {
      printf("PLAY request is missing object (path/file) parameter.\n");
      remove_msg(hl + bl);
      send_reply(400, 0);		/* bad request */
      return (0);
    }
    if (!parse_url(objurl, server_name, &port, object)) {
      printf("Mangled URL in PLAY.\n");
      discard_msg();
      send_reply(400, 0);		/* bad request */
      return (0);
    }


    /* find a match with existing streams using the session ID. */
    if (DescriptionFormat == DESCRIPTION_SDP_FORMAT) {
      if (!get_int(in_buffer, hl, "Session", ":\t",
		 (int *) &StreamSessionID, 0)) {
        printf("ALERT: PLAY request missing Session setting.");
        remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
        return (0);
      }
      for (found = 0, i = 0; (!found) && (i < MAXSTREAMSESSIONS); i++)
        found = (s_state.sessionlist[i] == StreamSessionID);
      i--;

    } 
    else {

      if (!DescribedFlag) {
        found = 1;

        if (strstr(object, "../")) {
	printf("SETUP request specified an object parameter with a path "
	         "that is not allowed. '../' not permitted in path.\n");
	/* unsupported media type */
	send_reply(403, 0);
	return (0);
        }

        p = strrchr(object, '.');
        if (!p || (strcasecmp(p, ".WAV") && strcasecmp(p, ".AU"))) {
	printf("SETUP request specified an object (path/file) parameter "
	         "that is not a .WAV file.\n");
	/* unsupported media type */
	send_reply(415, "Accept: *.WAV objects only.\n");
	return (0);
        }
        PathName = alloc_path_name(BasePath, object);
        /* open the file. */
        if ((fd = open(PathName, O_RDONLY)) == -1) {
	printf("SETUP request specified an object (path/file) parameter "
	         "that could not be found.  (%s)\n", PathName);
	free(PathName);
	send_reply(404, 0);		/* not found */
	return (0);
        }
        free(PathName);

        free_streams(&s_state);
        /* allocate a stream descriptor */
        s_state.streams = calloc(1, sizeof(STREAM));
        s_state.settings.num_streams = 1;
        s_state.streams->stream_id = 0;
        strncpy(s_state.streams->filename, object,
	        sizeof(s_state.streams->filename));
        /* now process the RIFF file. */
        if (!get_RIFF_header(fd, s_state.streams)) {
	close(fd);
	printf("SETUP method could not find a valid RIFF header in the .WAV "
	         "file.\n");
	send_reply(415, "Accept: RIFF file format only.\n");
	return (0);
        }
        s_state.settings.max_bit_rate = s_state.streams->max_bit_rate;
        s_state.settings.ave_bit_rate = s_state.streams->ave_bit_rate;
        s_state.settings.max_pkt_size = s_state.streams->max_pkt_size;
        s_state.settings.ave_pkt_size = s_state.streams->ave_pkt_size;
        s_state.settings.duration = s_state.streams->duration;

        s = s_state.streams;
        close(fd);
      }
      else
        /* It's xrtsp-mh and has been described before. */
        for (s = s_state.streams, found = 0; s; s = s->next) {
	if (!strcasecmp(s->filename, object)) {
	    found = 1;
	    break;
	}
        }
    }

    if (!found) {
      printf("PLAY request must be made on an object (path/file) that has been SETUP.\n");
      remove_msg(hl + bl);	/* remove remainder of message from in_buffer */
      send_reply(400, "PLAY object must be SETUP first.\n");	/* bad request */
      return (0);
    }

    if (DescriptionFormat == DESCRIPTION_SDP_FORMAT)
      for (s = s_state.streams; s && (s->stream_id !=
				      s_state.sessionlist_stream_ids[i]); s = s->next);

    /* get smpte play range */
    input_offset = s->start_offset;
    hr = min = sec = 0;
    if ((p = get_string(in_buffer, hl, "Range", ": \t"))) {
      if ((p = get_string(p, hl - (p - in_buffer), "smpte", "= \t"))) {
        sscanf(p, "%d:%d:%d", &hr, &min, &sec);
        time_index = ((hr * 3600) + (min * 60) + sec) * 1000;
        if (time_index < s->duration) {
	input_offset += s->byte_count * time_index / s->duration;
        } else {
	printf("PLAY range is invalid.\n");
	remove_msg(hl + bl);
	send_reply(457, 0);	/* invalid range */
	return (0);
        }
      }
    }
    remove_msg(hl + bl);

    if (s->streamer && schedule_check(0L)) {
      streamer = (struct STREAMER *) s->streamer;
      if (s_state.flags & SS_CLIP_PAUSED)
        s_state.flags &= ~SS_CLIP_PAUSED;	/* remove pause */
      else
        streamer->input_offset = input_offset;
      lseek(streamer->input_fd, streamer->input_offset, SEEK_SET);
      if (s_state.cur_state != PLAY_STATE)
        resume_stream(streamer);	/* resume if not currently playing */
    } else {
      /* startup stream. */
      save_offset = s->start_offset;
      s->start_offset = input_offset;
      streamer = start_stream(0, s, s_state.settings.data_port);
      s->start_offset = save_offset;
      if (streamer == 0) {
        printf("ALERT: start_stream() error.\n");
        /* Internal server error */
        send_reply(500, "Bad configuration file.\n");
        return (0);
      }
    }

    send_reply(200, 0);
    printf("PLAY response sent.\n");
    return (1);
}

int
handle_pause_request(char *b)
{
    STREAM         *s;
    char            object[512];
    int             found;

    char            server_name[256];
    u_short         port;
    char            objurl[256];

    printf("PAUSE request received.\n");
    if (!sscanf(b, " %*s %254s ", objurl)) {
      printf("PAUSE request is missing object (path/file) parameter.\n");
      discard_msg();
      send_reply(400, 0);
      return (0);
    }
    if (!parse_url(objurl, server_name, &port, object)) {
      printf("Mangled URL in PLAY.\n");
      discard_msg();
      send_reply(400, 0);
      return (0);
    }
    discard_msg();
    for (s = s_state.streams, found = 0; s; s = s->next) {
      if (!strcasecmp(s->filename, object)) {
        found = 1;
        break;
      }
    }
    if (!found) {
      printf("PAUSE request must be made on an object (path/file) that has been SETUP.\n");
      send_reply(400, "PLAY object must be SETUP first.\n");	/* bad request */
      return (0);
    }
    if (s->streamer)
      stop_stream(s->streamer);

    s_state.flags |= SS_CLIP_PAUSED;

    send_reply(200, 0);
    printf("PAUSE response sent.\n");
    return (1);
}


int
handle_close_request(void)
{
    STREAM         *s;
    struct STREAMER *streamer;

    printf("TEARDOWN request received.\n");
    discard_msg();
    send_reply(200, 0);
    printf("TEARDOWN response sent.\n");
    for (s = s_state.streams; s; s = s->next) {
      streamer = (struct STREAMER *) s->streamer;
      if (streamer) {
        if (streamer->output_fd)
	udp_close(streamer->output_fd);
        stop_stream(streamer);
        free(streamer);
      }
    }

    s_state.flags &= 0;
    free_streams(&s_state);

    return (1);
}


/*********************************
 *      server special handling.
 ********************************/
int
udp_data_recv()
{
    printf("Panic: server called udp_data_recv()\n");
    return 0;
}



/***************************************
 *         server state machine
 **************************************/

void
handle_event(int event, int status, char *buf)
{
    static char    *sStateErrMsg = "State error: event %d in state %d\n";

    switch (s_state.cur_state) {
    case INIT_STATE:
      {
        switch (event) {
        case RTSP_GET_METHOD:
	handle_get_request(buf);
	break;

        case RTSP_SETUP_METHOD:
	if (handle_setup_request(buf))
	    s_state.cur_state = READY_STATE;
	break;

        case RTSP_CLOSE_METHOD:
	handle_close_request();
	DescribedFlag = 0;
	break;

        case RTSP_HELLO_METHOD:
	handle_hello_request();
	break;

        case RTSP_HELLO_RESPONSE:
	handle_hello_reply(status);
	break;

        case RTSP_PLAY_METHOD:
        case RTSP_PAUSE_METHOD:
	send_reply(455, "Accept: OPTIONS, DESCRIBE, SETUP, TEARDOWN\n");
	break;

        default:
	printf(sStateErrMsg, event, s_state.cur_state);
	discard_msg();	
	send_reply(501, "Accept: OPTIONS, DESCRIBE, SETUP, TEARDOWN\n");
	break;
        }
        break;
      }				/* INIT state */

    case READY_STATE:
      {
        switch (event) {
        case RTSP_PLAY_METHOD:
	if (handle_play_request(buf))
	    s_state.cur_state = PLAY_STATE;
	break;

        case RTSP_SETUP_METHOD:
	if (!handle_setup_request(buf))
	    s_state.cur_state = INIT_STATE;
	break;

        case RTSP_CLOSE_METHOD:
	if (handle_close_request())
	    s_state.cur_state = INIT_STATE;
	DescribedFlag = 0;
	break;

        case RTSP_HELLO_METHOD:
	handle_hello_request();
	break;

        case RTSP_PAUSE_METHOD:	/* method not valid this state. */
	send_reply(455, "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n");
	break;

        case RTSP_GET_METHOD:
	handle_get_request(buf);
	break;

        case RTSP_HELLO_RESPONSE:
	handle_hello_reply(status);
	break;

        default:
	printf(sStateErrMsg, event, s_state.cur_state);
	discard_msg();
	send_reply(501, "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n");
	break;
        }
        break;
      }				/* READY state */

    case PLAY_STATE:
      {
        switch (event) {
        case RTSP_PLAY_METHOD:
	if (!handle_play_request(buf))
	    s_state.cur_state = READY_STATE;
	break;

        case RTSP_PAUSE_METHOD:
	if (handle_pause_request(buf))
	    s_state.cur_state = READY_STATE;
	break;

        case RTSP_CLOSE_METHOD:
	if (handle_close_request())
	    s_state.cur_state = INIT_STATE;
	DescribedFlag = 0;
	break;

        case RTSP_HELLO_METHOD:
	handle_hello_request();
	break;

        case RTSP_HELLO_RESPONSE:
	handle_hello_reply(status);
	break;

        case RTSP_GET_METHOD:
	if (!handle_get_request(buf))
	    s_state.cur_state = INIT_STATE;
	break;

	case RTSP_SETUP_METHOD:
	if (handle_setup_request(buf))
	    s_state.cur_state = READY_STATE;
	else
		s_state.cur_state = INIT_STATE;
	break;


        default:
	printf(sStateErrMsg, event, s_state.cur_state);
	discard_msg();
	send_reply(501, "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n");
	break;
        }
        break;
      }				/* PLAY state */

    default:	
      {
        printf("State error: unknown state=%d, event code=%d\n",
	       s_state.cur_state, event);
        discard_msg();	
      }
      break;
    }				/* end of current state switch */
}
