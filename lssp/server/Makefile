CC=gcc
CFLAGS=-Werror -Wall -g -I/usr/local/include -I.
LFLAGS=-lm

.c.o:
	$(CC) -c $(CFLAGS) $<

OBJS=	\
	main.o 		\
	config.o	\
	parse.o		\
	msg_handler.o	\
	util.o	\
	eventloop.o	\
	socket.o	

rtsp-server: $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $(OBJS) -o lssp-server

clean:
	rm -f *.o core lssp-server lssp-server.core

main.o: main.c
config.o: config.c
parse.o: parse.c
msg_handler.o: msg_handler.c
eventloop.o: eventloop.c
socket.o: socket.c
util.o: util.c
