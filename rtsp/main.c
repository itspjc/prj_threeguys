#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "parser.h"
#include "eventloop.h"

void error_handling(char *message);

int main(int argc, char **argv){
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t optlen, adr_sz;
	pid_t pid;
	FILE *file;

	int option, str_len;
	char buf[BUF_SIZE];

    static u_short port = 3005;
    char base_path [256] = "";
    /* 정상적으로 서버 시작 */
    




    printf("==============================");
    printf("ThreeGuys RTSP Server Start \n");
    
    FILE * f;
    char *default_path;
    default_path = "config.cfg";

    f = fopen(default_path, "r");

    if(!f){
        printf("Can't open configuration file '%s'.", default_path);
        printf("Configuration file must be on same directory with excutable file \n");
        return 0;
    }

    while (!feof(f)) {

        char linebuf[256];

        if(!fgets(linebuf, sizeof(linebuf), f))
            break;

        if(linebuf[0]=='#' || linebuf[0]=='\n' || linebuf[0] == ' ')
            continue;

        if(strncasecmp(linebuf, "port=", 5) == 0) {
            char *tok = strtok(linebuf, "=");
            tok = strtok(NULL, " \n");
            port = (u_short) atol(tok);
            continue;
        }

        if(strncasecmp(linebuf, "media_path", 10) == 0) {
            char *tok = strtok(linebuf, "=");
            tok = strtok(NULL, " \n");
            strncpy(base_path, tok, sizeof(base_path)-1);
            continue;
        }

    }
                
    printf("RTSP port : %d. \n", port);
    printf("Default Media Path : %s. \n", base_path);
    printf("==============================");

    


    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr,0,sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(port);

	optlen = sizeof(option);
	option = 1;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&option, optlen);

	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if(listen(serv_sock, 5) == -1)
		error_handling("listen() erorr");

	while(1) {

	    adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
		rtsp_sock = clnt_sock;

        // 경로를 받아서 지정

		file = fopen("media/sample.ts", "rb");
//		file = fopen(strcat(base_path, "sample.ts"), "rb");
//      printf("%s\n", base_path);		
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
