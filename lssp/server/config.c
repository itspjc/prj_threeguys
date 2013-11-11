
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "header/machdefs.h"
#include "header/rtsp.h"
#include "header/config.h"
#include "header/util.h"

extern void terminate( int code );

#define MAXURLS 10

struct url_info *urls[MAXURLS];
static int n_urls = 0;
static u_short port = RTSP_DEFAULT_PORT;
char BasePath [MAX_BASE_PATH] = "";

u_short
get_config_port()
{
     return port;
}

char *
get_media_path()
{
    return BasePath;
}

void
init_config(int argc, char **argv)
{
        /* example.cfg 파일을 읽어서 포트와 미디어파일 경로 설정
         * 1. example.cfg 파일은 프로젝트 홈디렉토리
         * 2. 기본 미디어 경로는 ./media/
         * */
        
    FILE *f;
    int i;

    for (i = 0; i < 10; i++) // What is urls ?
         urls[i] = 0;

    /* Read Config File */

    char *default_path; // Fix media_path now.
    default_path = "example.cfg";

    f = fopen(default_path, "r"); // open config file ( example.cfg ) 

    if (!f)
    {
      printf( "Can't open configuration file '%s'.", default_path );
      printf( "Configuration file must be on same directory with executable file \n" );
      terminate(-1);
    }

    i = 0;

    while (!feof(f))
    {
        char linebuf[256];

        if(!fgets(linebuf, sizeof(linebuf), f))
            break;
        
        if(linebuf[0] == '#' || linebuf[0] == '\n' || linebuf[0] == ' ')
            continue;

        if(strncasecmp(linebuf, "port=", 5) == 0)
        {
            /* skip 'port:' token */
            char *tok = strtok(linebuf, "=");
            tok = strtok(NULL, " \n");
            port = (u_short)atol(tok);
            continue;
        }

        if(strncasecmp(linebuf, "media_path=", 10) == 0)
        {
            /* skip 'port:' token */
            char *tok = strtok(linebuf, "=");
            tok = strtok(NULL, " \n");
            strncpy( BasePath, tok, sizeof(BasePath) - 1 );
            continue;
        }
    }
}

struct url_info *
get_url_info(const char *urlpath)
{
     int i;
     for(i=0;i<n_urls;i++)
     {
	if(strcmp(urls[i]->urlpath, urlpath) == 0)
	     return urls[i];
     }
     return 0;
}
