#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "rtp.h"
#include "streamer.h"

STREAMER* initStreamer(const int rtsp_sock, const int rtp_port, const int rtcp_port, const int transportMode){

	STREAMER* streamer = malloc(sizeof(STREAMER));

    struct sockaddr_in recvAddr;  
	struct sockaddr_in sendRtpAddr;
	struct sockaddr_in sendRtcpAddr;
	int sendRtpLen = sizeof(sendRtpAddr);
	int sendRtcpLen = sizeof(sendRtcpAddr);

	struct timeval init = { 0, 0 };

	gettimeofday(&init, 0);
	streamer->init_sec = init.tv_sec;

	getpeername(rtsp_sock, (struct sockaddr *)&sendRtpAddr, &sendRtpLen);
	sendRtpAddr.sin_family = AF_INET;
	sendRtpAddr.sin_port   = htons(rtp_port);

	getpeername(rtsp_sock, (struct sockaddr *)&sendRtcpAddr, &sendRtcpLen);
	sendRtcpAddr.sin_family = AF_INET;
	sendRtcpAddr.sin_port   = htons(rtcp_port);

	memcpy((void *)&(streamer->sendRtpAddr),(void *) &sendRtpAddr, sendRtpLen);
	memcpy((void *)&(streamer->sendRtcpAddr),(void *) &sendRtcpAddr, sendRtcpLen);
	streamer->pauseSet = 0;
	streamer->pauseTime = 0;
	
	recvAddr.sin_family      = AF_INET;   
	recvAddr.sin_addr.s_addr = INADDR_ANY;   
	
    int port;
	int rtp_sock, rtcp_sock;

    
    for (port = 6970; port < 7000 ; port += 2)
    {
		if(!transportMode) // 0x0 : TCP
		{
		    rtp_sock = socket(AF_INET, SOCK_STREAM, 0);
			if(connect(rtp_sock, (struct sockaddr*)&sendRtpAddr, sizeof(sendRtpAddr)) < 0)
				printf("Connect error\n");
        }
		else
            rtp_sock = socket(AF_INET, SOCK_DGRAM, 0);                     
        
        recvAddr.sin_port = htons(port);
  	    
        
        if (bind(rtp_sock,(struct sockaddr*)&recvAddr,sizeof(recvAddr)) == 0)
        {
			if(!transportMode)		
			{
				rtcp_sock = socket(AF_INET, SOCK_STREAM, 0);
				if(connect(rtcp_sock, (struct sockaddr*)&sendRtcpAddr, sizeof(sendRtcpAddr)) < 0)
					printf("Connect error\n");
			}
			else
				rtcp_sock = socket(AF_INET, SOCK_DGRAM, 0);

    		recvAddr.sin_port = htons(port + 1);

			if (bind(rtcp_sock,(struct sockaddr*)&recvAddr,sizeof(recvAddr)) == 0)
            {	
				streamer->rtp_sock = rtp_sock;
				streamer->rtcp_sock = rtcp_sock;
				streamer->serverRTPPort = port;
				streamer->serverRTCPPort = port+1;
				break; 
     	  	}
            else 
            {
                printf("All Failed? \n");
				close(rtp_sock);
				close(rtcp_sock);
			}
		}
        else
        {
			close(rtp_sock);
        }
	}

	streamer->sequenceNo = 1;

	return streamer;
}

void buildRTPHeader(STREAMER* streamer, char* rtppacket, RTP_PKT* rtp_pkt){
    struct timeval now;
	gettimeofday(&now, 0);

   	rtp_pkt->header.version = 2;
	rtp_pkt->header.extension = 0;
   	rtp_pkt->header.padding = 0;
   	rtp_pkt->header.csrc_len = 0;

   	rtp_pkt->header.marker = 1;
   	rtp_pkt->header.payload = RTP_PAYLOAD_MPEG2_TS;

	rtp_pkt->header.seq_no = htons(streamer->sequenceNo++);
   	rtp_pkt->header.timestamp = htonl((now.tv_sec - streamer->init_sec) * 1000 + (now.tv_usec / 1000));
   	rtp_pkt->header.ssrc = streamer->sessionID;

	memcpy(rtppacket+4, (void *)rtp_pkt, 12);
}

void playStream(STREAMER* streamer){
	int i, j, t, p, bitCount = 0;
	char buffer[255];
	char rtppacket[sizeof(RTP_PKT) + 4];
	time_t startTime_t, currentTime;	
	time(&startTime_t);
	streamer->startTime = startTime_t;
	printf("startTime : %d\n", streamer->startTime);
	while(1){
		time(&currentTime);
		while( bitCount > (currentTime-startTime_t)*(4*1024*1024 + 400000) ){
		usleep(100000);
		time(&currentTime);
		}
		
		memset(&rtp_pkt, 0, sizeof(RTP_PKT));
		memset(buffer, 0, sizeof(buffer));
		memset(rtppacket, 0, sizeof(rtppacket));
		rtppacket[0] = '$';
		rtppacket[1] = 0;
		rtppacket[2] = (sizeof(rtppacket) & 0x0000FF00) >> 8;
		rtppacket[3] = (sizeof(rtppacket) & 0x000000FF);
		buildRTPHeader(streamer, rtppacket, &rtp_pkt);
		
		p = 16;
		for(i = 0; i < 3; i++){
			memset(buffer, 0, sizeof(buffer));
			fread(buffer, TS_PACKET_SIZE, 1, streamer->input);
            for(j = 0; j < TS_PACKET_SIZE; j++){
				rtppacket[p++] = buffer[j];
            }
		}


/*		for(i = 0; i < 16; i++){
			printf("%2x\t", rtppacket[i] & 0xFF);
		}
		printf("\n");

		for(i = 0; i < TS_PACKET_SIZE; i++){
			printf("%2x\t", rtppacket[i+16] & 0xFF);
			if((i+1)%16 == 0)
				printf("\n");
		}
		printf("\n");
		for(i = 0; i < TS_PACKET_SIZE; i++){
			printf("%2x\t", rtppacket[i+16+TS_PACKET_SIZE] & 0xFF);
			if((i+1)%16 == 0)
				printf("\n");
		}
		printf("\n");
		for(i = 0; i < TS_PACKET_SIZE; i++){
			printf("%2x\t", rtppacket[i+16+2*TS_PACKET_SIZE] & 0xFF);
			if((i+1)%16 == 0)
				printf("\n");
		}*/

		if(!streamer->transportMode) // 0x0 : TCP
        {
            printf("first\n");
		    int errorno = write(streamer->rtp_sock,rtppacket,sizeof(rtppacket));
			printf("tcp called : %d error no : %d\n", t, errorno);
		}
        else
        {
			int errorno = sendto(streamer->rtp_sock, rtppacket+4, sizeof(RTP_PKT), 0, (struct sockaddr*)&(streamer->sendRtpAddr), sizeof(streamer->sendRtpAddr));
			bitCount = bitCount + 4608;// which means (12+188*3) * 8 = (bytes per one packet) * (8 bit per byte)	
	//	printf("udp called : %d error no : %d send it to port : %d\n", t, errorno, ntohs(streamer->sendRtpAddr.sin_port));
		}
	while(streamer->pauseSet){;}	

	}

	printf("\n\n\n\n While Finished \n\n\n\n");
}

void pauseStream(STREAMER *streamer){
	time_t currentTime;
	streamer->pauseSet = 1;
	time(&currentTime);
	streamer->pauseTime = currentTime - (streamer->startTime);

	printf("currentTime : %d, StartTime : %d, pauseTime : %d\n", currentTime, streamer->startTime, streamer->pauseTime);	
	//kill(streamer->playpid, SIGQUIT);
	//time(&(streamer->pauseTime));
}

void removeStreamer(STREAMER *streamer){
	
	printf(" process of pid %d will be killed ", streamer->playpid);
	kill(streamer->playpid, SIGQUIT);
	free(streamer);
}

