all: client server

server: server.o
	$(CC) -o server server.o -D_GNU_SOURCE -lhiredis

server.o: server.c
	$(CC) -c server.c