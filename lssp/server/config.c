
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

#include "machdefs.h"
#include "rtsp.h"
#include "config.h"
#include "util.h"

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

void
init_config(int argc, char **argv)
{
    FILE *f;
    int i;

    if (argc < 2) // If there are no arguments, print Error message.
    {
        printf("No configuration file specified.  "
                 "usage: rtsp-server <config file>\n");
        return;
    }

    for (i = 0; i < 10; i++) // What is urls ?
         urls[i] = 0;

    f = fopen(argv[1], "r"); // open config file ( ../etc/example.cfg ) 
    if (!f)
    {
      printf( "Can't open configuration file '%s'.", argv[1] );
      terminate(-1);
    }

    i = 0;

    /* Read Config File */
    while (!feof(f))
    {
        char linebuf[256];

        if(!fgets(linebuf, sizeof(linebuf), f))
            break;
        if(linebuf[0] == '#' || linebuf[0] == '\n' || linebuf[0] == ' ')
            continue;

        if(strncasecmp(linebuf, "port:", 5) == 0)
        {
            /* skip 'port:' token */
            char *tok = strtok(linebuf, ":");
            tok = strtok(NULL, " \n");
            port = (u_short)atol(tok);
            continue;
        }

        if(strncasecmp(linebuf, "BasePath=", 9) == 0)
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
