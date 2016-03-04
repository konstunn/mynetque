
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
#include "myqueue.h"
#include "message.pb-c.h"

#define N 10
#define K 10
#define L 10

int g_debug = 1;

extern char* program_invocation_short_name;

// TODO: test if executing atomically
static void dbg(const char *fmt, ...) {
	va_list args;

	if (g_debug) {
		va_start(args, fmt);
		flockfile(stderr);
		fflush(NULL);
		
		// TODO: get pid/tid and print it as well
		fprintf(stderr, "%s: ", program_invocation_short_name); 

		vfprintf(stderr, fmt, args);
		fprintf(stderr, "\n");
		fflush(NULL);
		funlockfile(stderr);
		va_end(args);
	}
	return;
}

struct bcast_thread_arg {
	int KL;
	int type; 
};

bool bcast_condition1() {
	return myqueue_count() < myqueue_max_count();
}

bool bcast_condition2() {
	return myqueue_count() > 0; 
}

void* bcast_thread_func(void *p)
{
	struct bcast_thread_arg *bta = (struct bcast_thread_arg*) p;	
	const int KL = bta->KL;	
	const int type = bta->type;

	const int sfd = socket(AF_INET, SOCK_DGRAM, 0);

	int bcast_enable = 1;
	if (setsockopt(sfd,	SOL_SOCKET, SO_BROADCAST, & bcast_enable, sizeof(bcast_enable)) < 0)
		err(EXIT_FAILURE, "setsockopt broadcast failed"); 

	struct sockaddr_in addr;
	memset(& addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT); 
	addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);	

	if (connect(sfd, (struct sockaddr*) & addr, sizeof(addr)) < 0)
		err(EXIT_FAILURE, "connect() failed - failed to set default send address");

	bool (*bcast_condition)();

	switch (type) {
		case 1:
			bcast_condition = bcast_condition1;
			break;
		case 2:
			bcast_condition = bcast_condition2;
	}

	const int bcast_type = type;

	struct timespec ts1;
	struct timespec ts2;

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, & ts1);

	while (1) // working loop
	{
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, & ts2);

		if (bcast_condition())
		{
			if (ts2.tv_sec - ts1.tv_sec >= KL)
			{
				if (write(sfd, & bcast_type, sizeof(int)) < 0)
					warn("write() broadcast failed");
				else {
					dbg("que cnt = %d, bcast type %d sent", myqueue_count(), bcast_type);
				}

				ts1.tv_sec = ts2.tv_sec;
			} 
		}
		else 
			ts1.tv_sec = ts2.tv_sec;
	}

	return NULL;
}

void* client_server_thread_func(void *p)
{
	pthread_detach(pthread_self());

	const int sfd = *((int*) p);
	free(p);

	int client_type = 0;

	// TODO: add magic sequence in the protocol 
	// or use some auth method ?
	
	// read client type
	if (read(sfd, & client_type, sizeof(int)) < 0) {
		warn("failed to receive client type");
		close(sfd);
		return NULL;
	}
		
	Message *pmessage;
	Message message;

	struct msg mesg;
	uint8_t data[MAX_MSG_DATA_LEN+1]; 
	mesg.data = (uint8_t*) data;

	uint8_t *buf;

	int msg_len;
	
	if (client_type == 1)
	{
		buf = (uint8_t*) malloc(MAX_MSG_SIZE);
			
		// recv mesg and push it into queue
		msg_len = read(sfd, buf, MAX_MSG_SIZE);

		pmessage = message__unpack(NULL, msg_len, buf);
		mesg.T = pmessage->t;
		mesg.len = pmessage->data.len;
		mesg.data = pmessage->data.data;

		myqueue_push(& mesg);

		if (g_debug) {
			memcpy(buf, mesg.data, mesg.len);
			buf[mesg.len] = '\0';
			dbg("rcvd \"%s\", len = %d, T = %d", buf, mesg.len, mesg.T);
		}

		message__free_unpacked(pmessage, NULL);
		free(buf);

	} else if (client_type == 2) {
		message__init(& message);	
		
		// pop a mesg from a queue and send it to client
			
		myqueue_front(& mesg);	

		// TODO: add myqueue_front_pop() function to myqueue.c
		// that would perform front and pop atomically
		myqueue_pop();			

		message.t = mesg.T;
		message.data.len = mesg.len;
		message.data.data = (uint8_t*) mesg.data;

		int msg_size = message__get_packed_size(& message);
		buf = (uint8_t*) malloc(msg_size);
		message__pack(& message, buf);

		write(sfd, buf, msg_size);

		if (g_debug) {
			memcpy(buf, mesg.data, mesg.len);
			buf[mesg.len] = '\0';
			dbg("sent \"%s\", len = %d, T = %d", buf, mesg.len, mesg.T);
		}	

		free(buf);
	}
	else 	
		warn("invalid client type");

	close(sfd);		
	return NULL;
}


int main()
{
	myqueue_init(N);

	int tcp_sfd = socket(AF_INET, SOCK_STREAM, 0);
	
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);	
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(tcp_sfd, (struct sockaddr *) & addr, sizeof(addr)) == -1)
		err(EXIT_FAILURE, "bind() failed");	

	if (listen(tcp_sfd, SOMAXCONN) < 0)
		err(EXIT_FAILURE, "listen() failed");
	
	// create threads that would notify clients on udp 
	struct bcast_thread_arg bta1, bta2;
	bta1.type = 1;
	bta2.type = 2;
	bta1.KL = K;
	bta2.KL = L;
	pthread_t tid;
	pthread_create(& tid, NULL, bcast_thread_func, & bta1);
	pthread_create(& tid, NULL, bcast_thread_func, & bta2);

	// TODO: catch and handle signals 
	// sigterm, sigint - destroy que!, close fd's, and other clean up stuff)

	int cfd;
	while (1) // working loop
	{
		if ((cfd = accept(tcp_sfd, NULL, NULL)) < 0) {
			warn("accept() failed");
			continue;
		}

		// create a thread to process accepted connection
		int *p = (int*) malloc(sizeof(int)); // thread would free it
		*p = cfd;
		pthread_create(& tid, NULL, client_server_thread_func, p);
	}

	// FIXME: this code is unreachable
	myqueue_destroy();
	
	return 0; 
}
