#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>

#include "eventloop.h"
#include "parser.h"
#include "rtp.h"
#include "streamer.h"

void handle_option()
{
    sprintf(response,
			"RTSP/1.0 200 OK\r\n"
			"CSeq: %s\r\n"
			"Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n\r\n",cseq);

	printf("-------------S -> C-------------\n"
			"%s\n", response);

    write(rtsp_sock, response, strlen(response));   
}

void getDate(char *date){
	char buf[256];
    time_t t = time(NULL);
    strftime(buf, sizeof(buf), "%a %b %d %Y %H:%M:%S GMT", gmtime(&t));
	strcpy(date, buf);
}

void handle_describe()
{    
	sprintf(SDPbuf,
		"v=0\r\n"
        "o=- %d 1 IN IP4 %s\r\n"           
        "s=\r\n"
        "t=0 0\r\n"                            
        "m=video 0 RTP/AVP %d\r\n"
        "c=IN IP4 0.0.0.0\r\n",
        rand(),
        hostaddr,
		RTP_PAYLOAD_MPEG2_TS);

	sprintf(URLbuf,
        "rtsp://%s/%s",
        hostaddr,
        filename);

	getDate(date);
	sprintf(response,
        "RTSP/1.0 200 OK\r\n"
		"CSeq: %s\r\n"
        "Date: %s\r\n"
        "Content-Base: %s/\r\n"
        "Content-Type: application/sdp\r\n"
        "Content-Length: %d\r\n\r\n"
       	"%s",
        cseq,
        date,
        URLbuf,
        strlen(SDPbuf),
        SDPbuf);

	printf("-------------S -> C-------------\n"
			"%s\n", response);
	write(rtsp_sock, response, strlen(response));
}

void handle_setup(){
	rtspSessionID = rand();
	getDate(date);

	if(!clientRTPPort)
		clientRTPPort = RTP_CLT_PORT_NO;
	if(!clientRTCPPort)
		clientRTCPPort = RTCP_CLT_PORT_NO;

	streamer = initStreamer(rtsp_sock, clientRTPPort, clientRTCPPort, transportMode);

    if (transportMode)
        sprintf(transport,"RTP/AVP/TCP;unicast;interleaved=0-1");
    else
        sprintf(transport,
            "RTP/AVP;unicast;destination=192.168.0.6;source=192.168.0.6;client_port=%i-%i;server_port=%i-%i",
            clientRTPPort,
            clientRTCPPort,
            streamer->serverRTPPort,
            streamer->serverRTCPPort);
    sprintf(response,
        "RTSP/1.0 200 OK\r\n"
		"CSeq: %s\r\n"
        "Date: %s\r\n"
        "Transport: %s\r\n"
        "Session: %i\r\n\r\n",
        cseq,
        date,
        transport,
        rtspSessionID);
	
	streamer->input = inputStream; 
	streamer->sessionID = rtspSessionID;
	streamer->transportMode = transportMode;
	printf("-------------S -> C-------------\n"
		"%s\n", response);
    write(rtsp_sock,response,strlen(response));
}

void handle_play(){
	getDate(date);

    sprintf(response,
        "RTSP/1.0 200 OK\r\n"
		"CSeq: %s\r\n"
        "Date: %s\r\n"
        "Range: npt=0.000-\r\n"
        "Session: %i\r\n"
        "RTP-Info: url=rtsp://192.168.0.6:3005/muhan.avi\r\n\r\n",
        cseq,
        date,
        rtspSessionID);

	printf("-------------S -> C-------------\n"
		"%s\n", response);
    write(rtsp_sock,response,strlen(response));
	
	playStream(streamer);	
}

void handle_teardown(){
    sprintf(response,
        "RTSP/1.0 200 OK\r\n"
		"CSeq: %s\r\n",
        cseq);

	printf("-------------S -> C-------------\n"
		"%s\n", response);
    write(rtsp_sock,response,strlen(response));

	removeStreamer(streamer);
	close(rtsp_sock);
}

void handle_pause(){
	getDate(date);

    sprintf(response,
        "RTSP/1.0 200 OK\r\n"
		"CSeq: %s\r\n"
		"Date: %s\r\n",
        cseq,
		date);

	printf("-------------S -> C-------------\n"
		"%s\n", response);
    write(rtsp_sock,response,strlen(response));

	pauseStream(streamer);
}

void parse_rtsp(){
	int str_len, cnt = 0;
	char *p, *q;

	while((str_len = read(rtsp_sock, cmd, BUF_SIZE)) != 0){
		printf("-------------C -> S-------------\n"
			"%s\n", cmd);
		
		p = strtok(cmd, "\r\n");
		if (strstr(p,"OPTIONS") != NULL){
			rtspCmdType = RTSP_OPTIONS;
			p += 7;
		}else if (strstr(p,"DESCRIBE") != NULL){
			rtspCmdType = RTSP_DESCRIBE;
			p += 9;
		}else if (strstr(p,"SETUP") != NULL) {
			rtspCmdType = RTSP_SETUP;
			p += 6;
	   	}else if (strstr(p,"PLAY") != NULL) {
			rtspCmdType = RTSP_PLAY;
			p += 5;
   		}else if (strstr(p,"TEARDOWN") != NULL) {
			rtspCmdType = RTSP_TEARDOWN;
			p += 9;
		}else if(strstr(p, "PAUSE") != NULL){
			rtspCmdType = RTSP_PAUSE;
			p += 6;
		}else{
			printf("Command error\n");
			close(rtsp_sock);
			exit(-1);
		}

		if(strstr(p, "rtsp://") != NULL){
			p += 7;
			int i = 0;
			while(*p != '/')
				hostaddr[i++] = *(p++);
			hostaddr[i] = 0;

			i = 0;
			p++;
			while(*p != ' ')
				filename[i++] = *(p++);
			filename[i] = 0;
		}else{
			printf("URL error\n");
			close(rtsp_sock);
			exit(-1);
		}

		p = strtok(NULL, "\r\n");
		
		if ((p = strstr(p, "CSeq")) != NULL){
			p += 5;
			int i = 0;
			while(*p != 0)
				cseq[i++] = *(p++);
			cseq[i] = 0;
		} else{
			printf("CSeq error\n");
			close(rtsp_sock);
			exit(-1);
		}

		if (rtspCmdType == RTSP_SETUP)
    	{
			p = strtok(NULL, "\r\n");
			p = strtok(NULL, "\r\n");
        	if ( strstr(p,"RTP/AVP/TCP") != NULL)
				transportMode = TCP;
			else
				transportMode = UDP;
			if((p = strstr(p, "client_port=")) != NULL){
				p += 12;
				int i = 0;
				char port[25] = {0, };
				while(*p != '-')
					port[i++] = *(p++);
				
				port[i] = 0;
				clientRTPPort = atoi(port);
				
				i = 0;
				p++;
				while(*p != 0){
					if(*p != '\r' || *p != '\n')
						port[i++] = *(p++);
				}
				port[i] = 0;
				clientRTCPPort = atoi(port);
			}	
    	}
		if(rtspCmdType == RTSP_PLAY){
			while((str_len = read(streamer->rtcp_sock, cmd, BUF_SIZE)) > 0){
			
			}
		}
		switch(rtspCmdType){
		case RTSP_OPTIONS : handle_option(); break;
		case RTSP_DESCRIBE : handle_describe(); break;
		case RTSP_SETUP : handle_setup(); break;
		case RTSP_PLAY : handle_play(); break;
		case RTSP_TEARDOWN : handle_teardown(); break;
		case RTSP_PAUSE : handle_pause(); break;
		}
	}
}
