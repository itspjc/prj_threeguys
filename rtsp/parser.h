#ifndef __PARSER_H__
#define __PARSER_H__

#include "streamer.h"

#define BUF_SIZE 1024

#define RTSP_OPTIONS 	0x1
#define RTSP_DESCRIBE 	0x2
#define RTSP_SETUP 		0x3
#define RTSP_PLAY		0x4
#define RTSP_TEARDOWN	0x5
#define RTSP_PAUSE		0x6

#define RTP_CLT_PORT_NO 	5004
#define RTCP_CLT_PORT_NO 	5009

#define TCP 0x0
#define UDP 0x1

int rtspCmdType;
int transportMode;
int rtspSessionID;

int clientRTPPort;
int clientRTCPPort;

char hostaddr[256];
char filename[256];
char cseq[256];

STREAMER* streamer;
extern FILE* inputStream;
extern FILE* file;
void parse_rtsp();

#endif
