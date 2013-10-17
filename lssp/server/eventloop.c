
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "machdefs.h"
#include "eventloop.h"
#include "messages.h"
#include "msg_handler.h"
#include "scheduler.h"
#include "socket.h"


int Quit = 0;
u_long inow;	                     /* Internal time format (ms)		  */
void (*readers[MAX_FDS])(int);  /* Descriptor -> Function table (reading) */
int main_fd = 0;	          /* The primary TCP channel's fd		  */
int main_fd_read = 0;
int server = 0;            /* run-time check to control eventloop since the
                                    same object is used to build server and player. */
static int  in_eventloop = 0;

void
eventloop_init()
{
    int i;

    for (i = 0; i < MAX_FDS; i++)
        readers[i] = 0;
}

void
terminate(int code)
{
     exit(code);
}

void
eventloop()
{
    static struct timeval start = { 0, 0 };
    struct timeval t = { 0, 10000 };
    struct timeval now;
    fd_set readset;
    fd_set writeset;
    int i;
    
    if ( in_eventloop || Quit )
        return;      /* prevent re-entrant behaviour if called off timer event */
    
    in_eventloop = 1;
    
    /* The gettimeofday() function shall obtain the current time,
       expressed as seconds and microseconds since the Epoch, and store it 
       in the timeval structure pointed to by tp. The resolution of the system clock is unspecified. */
    
    if (!start.tv_sec)
        gettimeofday(&start, 0); 

    /* This function initializes the file descriptor set to contain no file descriptors. */
    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    if (main_fd)
    {
        assert( main_fd < FD_SETSIZE );
        if ( main_fd >= FD_SETSIZE )
        {
            printf( "socket fd exceeded FD_SETSIZE limit.\n" );
            terminate( 1 );
        }
        if ( !server )
            main_fd_read = main_fd;
        FD_SET( main_fd_read, &readset);
    }

    if ( !server )
        for (i = 0; i < MAX_FDS; i++)
            if (readers[i])
                FD_SET(i, &readset);

    if (main_fd && io_write_pending())
        FD_SET(main_fd, &writeset);

    select(FD_SETSIZE, &readset, &writeset, 0, &t);

    gettimeofday(&now, 0);
    inow = ((now.tv_sec - start.tv_sec) * 1000) + (now.tv_usec / 1000);

    schedule_execute(inow);

    if (main_fd && FD_ISSET(main_fd_read, &readset))
    {
        io_read( main_fd_read );
        msg_handler();
    }

    if ( !Quit )
    {
        if (main_fd && FD_ISSET(main_fd, &writeset))
            io_write(main_fd);

        if ( !server )
            for (i = 0; i < MAX_FDS; i++)
                if (FD_ISSET(i, &readset) && readers[i])
                    readers[i](i);
    }

    in_eventloop = 0;
}
