#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "rtp.h"
#include "streamer.h"

STREAMER* initStreamer(const int rtsp_sock, const int rtp_port, const int rtcp_port, const int transportMode){

	STREAMER* streamer = malloc(sizeof(STREAMER));

    struct sockaddr_in recvAddr;  
	struct sockaddr_in sendAddr;
	int sendLen = sizeof(sendAddr);

	struct timeval init = { 0, 0 };

	gettimeofday(&init, 0);
	streamer->init_sec = init.tv_sec;

	getpeername(rtsp_sock, (struct sockaddr *)&sendAddr, &sendLen);
	sendAddr.sin_family = AF_INET;
	sendAddr.sin_port   = htons(rtp_port);

	streamer->sendAddr = sendAddr;

	recvAddr.sin_family      = AF_INET;   
	recvAddr.sin_addr.s_addr = INADDR_ANY;   
	int port;
	int rtp_sock, rtcp_sock;
    for (port = 6970; port < 65534 ; port += 2) {
		if(transportMode)
		    rtp_sock = socket(AF_INET, SOCK_STREAM, 0);                     
		else
		    rtp_sock = socket(AF_INET, SOCK_DGRAM, 0);                     
    	recvAddr.sin_port = htons(port);
  	    if (bind(rtp_sock,(struct sockaddr*)&recvAddr,sizeof(recvAddr)) == 0) {
			if(transportMode)
				rtp_sock = socket(AF_INET, SOCK_STREAM, 0);
			else
	    		rtcp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    		recvAddr.sin_port = htons(port + 1);
			if (bind(rtcp_sock,(struct sockaddr*)&recvAddr,sizeof(recvAddr)) == 0) {
				streamer->rtp_sock = rtp_sock;
				streamer->rtcp_sock = rtcp_sock;
				streamer->serverRTPPort = port;
				streamer->serverRTCPPort = port+1;
				break; 
     	  	} else {
				close(rtp_sock);
				close(rtcp_sock);
			}
		} else 
			close(rtp_sock);
	}

	streamer->sequenceNo = 1;

	return streamer;
}

void buildRTPHeader(STREAMER* streamer, RTP_PKT* rtp_pkt){
    struct timeval now;
	gettimeofday(&now, 0);

   	rtp_pkt->header.version = 2;
	rtp_pkt->header.extension = 0;
   	rtp_pkt->header.padding = 0;
   	rtp_pkt->header.csrc_len = 0;

   	rtp_pkt->header.marker = 0;
   	rtp_pkt->header.payload = RTP_PAYLOAD_MPEG2_TS;

	rtp_pkt->header.seq_no = htons(streamer->sequenceNo++);
   	rtp_pkt->header.timestamp = htonl((now.tv_sec - streamer->init_sec) * 1000 + (now.tv_usec / 1000));
   	rtp_pkt->header.ssrc = htonl(streamer->sessionID);
}

void playStream(STREAMER* streamer){
	int i, j, t;
	char buffer[255];
    char buffer2[255];

	while (1) 
    {
		memset(&rtp_pkt, 0, sizeof(RTP_PKT));
		memset(buffer, 0, sizeof(buffer));
		buildRTPHeader(streamer, &rtp_pkt);

		for(i = 0; i < 5; i++){
			fread(buffer, TS_PACKET_SIZE, 1, streamer->input);
            for(j = 0; j < TS_PACKET_SIZE; j += 4){
                memcpy(buffer2[j], htonl(buffer[j]), 4);
            }
			memcpy(rtp_pkt.data[i], buffer2, TS_PACKET_SIZE);
		}

		if(streamer->transportMode)
		    write(streamer->rtp_sock,&rtp_pkt,sizeof(RTP_PKT));
		else
			sendto(streamer->rtp_sock, &rtp_pkt, sizeof(RTP_PKT), 0, (struct sockaddr*)&(streamer->sendAddr), sizeof(streamer->sendAddr));
	}
}

void pauseStream(STREAMER *streamer){

}

void removeStreamer(STREAMER *streamer){
	free(streamer);	
}

