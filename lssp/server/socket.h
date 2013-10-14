
#ifndef _SOCKET_H_
#define _SOCKET_H_

#define MAX_FDS      500

#define RTSP_SOCK int
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

extern struct sockaddr peer;

/* Initiate connection */
RTSP_SOCK tcp_open(struct sockaddr *name, int namelen);

/* Open / Listen on port */
RTSP_SOCK tcp_listen(unsigned short port);
RTSP_SOCK tcp_accept(RTSP_SOCK s, struct sockaddr *addr, socklen_t *addrlen);

/* Perform getpeername and log connection */
int inetd_init();

int tcp_read(RTSP_SOCK s, void *buffer, int nbytes);
int tcp_write(RTSP_SOCK s, void *buffer, int nbytes);
int tcp_close(RTSP_SOCK s);

/* Initiate connection */
RTSP_SOCK udp_open();
int udp_close(RTSP_SOCK sock);

/* Open / Listen on port */
RTSP_SOCK udp_listen(unsigned short port);

int udp_getport(RTSP_SOCK f, struct sockaddr_in *s);

/* Initiate connection */
RTSP_SOCK mcast_open(struct sockaddr *name, int namelen);
RTSP_SOCK mcast_listen(int port);

int dgram_recvfrom(RTSP_SOCK s, void *buf, size_t len, int flags,
    	struct sockaddr *from, socklen_t *fromlen);
int dgram_sendto(RTSP_SOCK s, const void *msg, size_t len, int flags,
	const struct sockaddr *to, int tolen);


#endif	/* _SOCKET_H_ */

