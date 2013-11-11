CC=gcc
CFLAGS=-Werror -Wall -g -I/usr/local/include -I.
LFLAGS=-lm

.c.o:
	$(CC) -c $(CFLAGS) $<

OBJS=	\
	main.o 		\
	server.o	\
	config.o	\
	streamer.o	\
	scheduler.o	\
	util.o		\
	parse.o		\
	msg_handler.o	\
	eventloop.o	\
	socket.o	

rtsp-server: $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $(OBJS) -o lssp-server

clean:
	rm -f *.o core lssp-server lssp-server.core

main.o: main.c
server.o: server.c
config.o: config.c
streamer.o: streamer.c
scheduler.o: scheduler.c
util.o: util.c
parse.o: parse.c
msg_handelr.o: msg_handler.c
eventloop.o: eventloop.c
socket.o: socket.c
