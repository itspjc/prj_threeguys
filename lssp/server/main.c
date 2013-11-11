#include <assert.h>
#include <stdio.h>

#include "header/machdefs.h"
#include "header/eventloop.h"
#include "header/socket.h"
#include "header/rtsp.h"
#include "header/config.h"
#include "header/server.h"

int
main (int argc, char **argv)
{
    //pid_t pid;
    int fd;
    int lfd;
    socklen_t size = 0;
    struct sockaddr_in s;
    u_short port;
    char *media_path;

    init_server(argc, argv); // Server initialize ( server.c )
    port = get_config_port(); // Return port number. ( config.c )
    media_path = (char *) get_media_path(); // Return media path ( config.c )

    printf("================================");
    printf( "ThreeGuys RTSP Server Start \n");
    printf( "RTSP port : %d.\n", port );
    printf( "Default media path :  %s.\n", media_path);
    printf("================================");
    
    /* tcp_listen : call listen () */

    lfd = tcp_listen(port); // socket.c, lfd : server socket fd.
    size = sizeof (s);
    
    
    for ( ; ; ) {
        // 멀티 프로세스 만들기 위해서 이부분을 수정해야 함

        fd = tcp_accept(lfd,(struct sockaddr *) &s, &size); // socket.c, fd : client socket.
        
        //if ( (pid = fork()) == 0 ) {
            
            // allocate client socket fd to main_fd.
            main_fd = fd;
            main_fd_read = fd;
            
            eventloop_init(); // eventloop.c , Description table 초기화인데 왜하는지 모르겠음
            
            while (1)
                eventloop(); // eventloop.c
    
            return 0;
            close(fd); // close server socket. 
        //}
    }
}
