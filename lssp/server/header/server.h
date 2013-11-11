
#ifndef _SERVER_H_
#define _SERVER_H_

#include "header/eventloop.h"
#include "header/socket.h"

/* external events */
#define STREAM_CLOSE	11000

void init_server(int argc, char **argv);

#endif /* _SERVER_H_ */

