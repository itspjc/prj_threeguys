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
    //pid_t pid;
    int fd;
    int lfd;
    socklen_t size = 0;
    struct sockaddr_in s;
    u_short port;

    init_server(argc, argv); // server.c 서버를 초기화하는 작업 
    port = get_config_port();

    printf( "RTSP port %d.\n", port );
    
    /* tcp_listen : call listen () */

    lfd = tcp_listen(port); // socket.c, lfd : server socket fd.

    size = sizeof (s); // To Do : recognize structure of sockaddr.
    
    
    for ( ; ; ) {
        // 멀티 프로세스 만들기 위해서 이부분을 수정해야 함

        fd = tcp_accept(lfd,(struct sockaddr *) &s, &size); // socket.c, fd : client socket.
        
        //if ( (pid = fork()) == 0 ) {
            // allocate client socket fd to main_fd.
            main_fd = fd;
            main_fd_read = fd;
            
            eventloop_init(); // eventloop.c , Description table 초기화인데 왜하는지 모르겠음
            
            while (1)
                eventloop();
    
            return 0;
            close(fd); // close server socket. 
        //}
    }
}
