CC      = gcc
CFLAGS  = -Wall -Wextra -o

all: server client

server: server.c
	$(CC) $(CFLAGS) server server.c

client: client.c
	$(CC) $(CFLAGS) client client.c

clean:
	rm -f server client received_*
