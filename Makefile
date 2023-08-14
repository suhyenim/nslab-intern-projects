all: client.out server.out

client.out : client.o
	gcc -o client.out client.o

client.o : client.c
	gcc -c -o client.o client.c

server.out : server.o
	gcc -o server.out server.o

server.o : server.c
	gcc -c -o server.o server.c

clean:
	rm *.o client.out server.out
