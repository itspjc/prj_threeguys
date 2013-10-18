#include <assert.h>
#include <stdio.h>

#include "machdefs.h"
#include "eventloop.h"
#include "socket.h"
#include "rtsp.h"
#include "config.h"
#include "server.h"

int
main (int argc, char **argv)
{
    pid_t pid;
    int fd;
    int lfd;
    socklen_t size = 0;
    struct sockaddr s = { 0 };
    u_short port;

    init_server(argc, argv); // server.h 
    port = get_config_port();

    printf( "RTSP port %d.\n", port );
    lfd = tcp_listen(port); // socket.c

    size = sizeof (s); // To Do : recognize structure of sockaddr.
    
    
    for ( ; ; ) {
        
        fd = tcp_accept(lfd, &s, &size); // socket.c
        
        if ( (pid = fork()) == 0 ) {
            main_fd = fd;
            main_fd_read = fd;
            eventloop_init(); // eventloop.c
            
            while (1)
                eventloop();
    
            return 0;
            close(fd);
        }
    }
}
