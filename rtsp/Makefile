rtsp : main.o parser.o eventloop.o streamer.o
	gcc -g -o rtsp main.o parser.o eventloop.o streamer.o

main.o : main.c parser.h eventloop.h
	gcc -c main.c

parser.o : parser.c parser.h eventloop.h streamer.h rtp.h
	gcc -c parser.c

eventloop.o : eventloop.c eventloop.h parser.h
	gcc -c eventloop.c

streamer.o : streamer.c streamer.h rtp.h
	gcc -c streamer.c
