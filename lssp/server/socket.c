
#include <syslog.h>
/*
#include <varargs.h>
*/
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>

#include <stdio.h>
#include <unistd.h>

#include "machdefs.h"
#include "session.h"
#include "socket.h"
#include "util.h"

extern int debug_toggle;
extern void terminate(int errorcode);


struct sockaddr peer;

int
tcp_open(struct sockaddr *name, int namelen)
{
    int f;

    if ((f = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      printf( "socket() error in tcp_open.\n" );
      terminate(-1);
    }

    if (connect(f, name, namelen) < 0)
    {
      printf( "connect() error in tcp_open.\n" );
      terminate(-1);
    }

    return f;
}

int
tcp_listen(unsigned short port)
{
    int f;
    struct sockaddr_in s;
    int v = 1;

    if ((f = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror( "socket() error in tcp_listen.\n" );
      terminate(-1);
    }

    setsockopt(f, SOL_SOCKET, SO_REUSEADDR, (char *) &v, sizeof(int));

    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(INADDR_ANY);
    s.sin_port = htons(port);

    if (bind (f, (struct sockaddr *)&s, sizeof (s)))
    {
      perror( "bind() error in tcp_listen" );
      terminate(-1);
    }

    if (listen(f, 5) < 0)
    {
      perror( "listen() error in tcp_listen.\n" );
      terminate(-1);
    }

    return f;
}

int
inetd_init()
{
   socklen_t namelen = sizeof (peer);
   struct hostent *hp;
   const char *lp;

   if (getpeername (0, &peer, &namelen))
   {
      printf( "getsockname() error in tcp_accept.\n" );
      terminate(-1);
   }
    if ((hp = gethostbyaddr(((const char *) &(peer.sa_data[2])),
        4, AF_INET)))
    { 
        lp = hp->h_name;
        syslog(LOG_ERR, "connection from %s", lp);
    }
    else
    {
        syslog(LOG_ERR, "connection from %x", *((int *) &(peer.sa_data[2])));
    }
    return(0);
}

int
tcp_accept(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
   int f;
   socklen_t namelen = sizeof (peer);
   struct hostent *hp;
   const char *lp;

   if ((f = accept (fd, addr, addrlen)) < 0)
   {
      printf( "accept() error in tcp_accept.\n" );
      terminate(-1);
   }
   if (getpeername (f, &peer, &namelen))
   {
      printf( "getsockname() error in tcp_accept.\n" );
      terminate(-1);
   }
    if ((hp = gethostbyaddr(((const char *) &(peer.sa_data[2])),
        4, AF_INET)))
    { 
        lp = hp->h_name;
        syslog(LOG_ERR, "connection from %s", lp);
    }
    else
    {
        syslog(LOG_ERR, "connection from %x", *((int *) &(peer.sa_data[2])));
    }
    return f;
}

int
tcp_read(int fd, void *buffer, int nbytes)
{
   int n;

   n = read(fd, buffer, nbytes);
   if ( debug_toggle )
   {
      printf ("Read:\n");
      dump_buffer(buffer, n);
   }
   
   return n;
}

int
tcp_write(int fd, void *buffer, int nbytes)
{
    int n;

   n = write(fd, buffer, nbytes);
   if ( debug_toggle )
   {
      printf ("Write:\n");
      dump_buffer(buffer, n);
   }

   return n;
}

int
tcp_close(int fd)
{
    return close(fd);
}

int
udp_open()
{
    int f;
    struct sockaddr_in s;
    int on = 1;

    if ((f = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
      printf( "socket() error in udp_open.\n" );
    	terminate(-1);
    }

    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(INADDR_ANY);
    s.sin_port = htons(INADDR_ANY);

    if (bind (f, (struct sockaddr *)&s, sizeof (s)))
    {
      printf( "bind() error in udp_open.\n" );
      terminate(-1);
    }
    /* set to non-blocking */
    if (ioctl(f, FIONBIO, &on) < 0)
    {
      printf( "ioctlsocket() error in udp_open.\n" );
      terminate(-1);
    }

    return f;
}

int
udp_close(int fd)
{
    return close(fd);
}

int
udp_getport(int f, struct sockaddr_in *s)
{
   socklen_t namelen = sizeof(struct sockaddr_in);

   if (getsockname (f, (struct sockaddr *) s, &namelen))
   {
      printf( "getsockname() error in udp_getport.\n" );
      terminate(-1);
   }
   return ntohs( s->sin_port );
}

int
udp_listen(unsigned short port)
{
    int f;
    struct sockaddr_in s;

    if ((f = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
      printf( "socket() error in udp_listen.\n" );
      terminate(-1);
    }

    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(INADDR_ANY);
    s.sin_port = htons(port);

    if (bind (f, (struct sockaddr *)&s, sizeof (s)))
    {
      printf( "bind() error in udp_listen.\n" );
      terminate(-1);
    }

    return f;
}

int
mcast_open(struct sockaddr *name, int namelen)
{
    return 0;
}

int
mcast_listen(int port)
{
    return 0;
}

int
dgram_recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr
	*from, socklen_t *fromlen)
{
    return recvfrom(s, buf, len, flags, from, fromlen);
}

int
dgram_sendto(int s, const void *msg, size_t len, int flags,
	const struct sockaddr *to, int tolen)
{
    return sendto(s, msg, len, flags, to, tolen);
}
