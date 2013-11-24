#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "parser.h"
#include "eventloop.h"

#define RTSP_PORT 3005

void error_handling(char *message);

int main(int argc, char **argv){
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t optlen, adr_sz;
	pid_t pid;
	FILE *file;

	int option, str_len;
	char buf[BUF_SIZE];
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr,0,sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(RTSP_PORT);

	optlen = sizeof(option);
	option = 1;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&option, optlen);

	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if(listen(serv_sock, 5) == -1)
		error_handling("listen() erorr");

	while(1)
	{
		adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
		rtsp_sock = clnt_sock;

		file = fopen("muhan.ts", "rb");
		if(file == NULL){
			printf("fopen error\n");
			return;
		}
		if(clnt_sock == -1)
			continue;
		else
			puts("new client connected ...");
		pid = fork();
		if(pid == -1){
			close(clnt_sock);
			continue;
		}
		if(pid == 0){
			close(serv_sock);
			rtsp_sock = clnt_sock;
			
			while(1)
				eventloop(file);

			close(clnt_sock);
		}
		else
			close(clnt_sock);
	}
	close(serv_sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
