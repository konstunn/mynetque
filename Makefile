
CC?=gcc
CFLAGS=-Wall -g -std=gnu99 -pthread
CFLAGS+=$(shell pkg-config --cflags 'libprotobuf-c >= 1.0.0')

LIBS=$(shell pkg-config --libs 'libprotobuf-c >= 1.0.0')

EXECS=server client1 client2

all: $(EXECS)

SERVER_OBJS = server.o myqueue.o message.pb-c.o

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

server : $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $@ $(LIBS)

clean:
	-@rm -vf $(EXECS) *.o

.PHONY: clean
