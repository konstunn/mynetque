
CC?=gcc
CFLAGS=-Wall -g -std=gnu99

EXECS=server client1 client2

all: $(EXECS)

SERVER_OBJS = server.o myqueue.o

%.o : %.c
	$(CC) $(CFLAGS) -c -pthread $< -o $@	

server : $(SERVER_OBJS)
	$(CC) $(CFLAGS) -pthread $(SERVER_OBJS) -o $@

clean:
	-@rm -vf $(EXECS) *.o

.PHONY: clean
