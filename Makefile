
CC?=gcc
CFLAGS=-Wall -g -std=gnu99 -pthread
CFLAGS+=$(shell pkg-config --cflags 'libprotobuf-c >= 1.0.0')

LIBS=$(shell pkg-config --libs 'libprotobuf-c >= 1.0.0')

EXECS=server client1 client2

SERVER_OBJS = server.o myqueue.o message.pb-c.o
CLIENT1_OBJS = client1.o message.pb-c.o
CLIENT2_OBJS = client2.o message.pb-c.o

all: $(EXECS)

%.pb-c.c : %.proto
	protoc-c --c_out=. $<

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

server.c : message.pb-c.c
client1.c : message.pb-c.c
client2.c : message.pb-c.c

server : $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $@ $(LIBS)

client1 : $(CLIENT1_OBJS)
	$(CC) $(CFLAGS) $(CLIENT1_OBJS) -o $@ $(LIBS)
	
client2 : $(CLIENT2_OBJS)
	$(CC) $(CFLAGS) $(CLIENT2_OBJS) -o $@ $(LIBS)

clean :
	-@rm -vf $(EXECS) *.o *.pb-c.*

.PHONY: clean
.PRECIOUS: %.pb-c.c
