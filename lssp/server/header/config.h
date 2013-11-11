
#include "header/socket.h"

void init_config(int argc, char **argv);

struct url_info
{
    char urlpath[1024];
    char realpath[1024];
    long file_len;
    int payload_type;
    int subtype;
    int bps;
    int max_pktsize;
    struct sockaddr_in mcast_addr;
};

struct url_info *get_url_info(const char *urlpath);
u_short get_config_port();
#define MAX_BASE_PATH 256
char BasePath [MAX_BASE_PATH];
