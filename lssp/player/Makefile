CC=gcc
CFLAGS=-Werror -Wall -g -I/usr/local/include -I. -I../server
LFLAGS=-lm

.c.o:
	$(CC) -c $(CFLAGS) $<

OBJS=	main.o 		\
	player.o	\
        ../server/scheduler.o     \
        ../server/util.o          \
        ../server/parse.o         \
        ../server/messages.o      \
        ../server/msg_handler.o   \
        ../server/eventloop.o     \
        ../server/socket.o

OBJS += text.o

rtsp-player: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LFLAGS) -o lssp-player

clean:
	rm -f *.o core lssp-player lssp-player.core

