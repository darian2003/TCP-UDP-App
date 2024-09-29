CFLAGS=-Wall
CC=gcc

build: server subscriber

server: server.c list.c queue.c
	$(CC) $(CFLAGS) server.c list.c queue.c -lm -o server

subscriber: subscriber.c list.c queue.c
	$(CC) $(CFLAGS) subscriber.c list.c queue.c -lm -o subscriber

clean:
	rm -f server subscriber