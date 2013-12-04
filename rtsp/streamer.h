#ifndef __STREAMER__H__
#define __STREAMER__H__

#include <stdio.h>
#include <arpa/inet.h>

#define FILENAME "muhan.ts"

typedef struct streamer
{
	struct sockaddr_in sendRtpAddr;
	struct sockaddr_in sendRtcpAddr;

	int rtp_sock;
	int rtcp_sock;

	FILE* input;

	int serverRTPPort;
	int serverRTCPPort;

	uint32_t sessionID;
	uint16_t sequenceNo;
	uint32_t init_sec;
	int transportMode;
} STREAMER;


STREAMER* initStreamer(const int rtsp_sock, const int rtp_port, const int rtcp_port, const int transportMode);
void playStream(STREAMER* streamer);
void pauseStream(STREAMER* streamer);
void removeStreamer(STREAMER* streamer);

#endif
