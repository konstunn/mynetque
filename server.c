
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <err.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "myqueue.h"

struct bcast_thread_arg {
	int udp_sfd;
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
	const int sfd = bta->udp_sfd;
	const int type = bta->type;

	struct sockaddr_in addr;
	memset(& addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(65535); 
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
			break;
		default:
			err(EXIT_FAILURE, "unknown bcast type");		
	}

	const int bcast_type = type;

	struct timespec ts1;
	struct timespec ts2;

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts1);

	while (1) // working loop
	{
		clock_gettime(CLOCK_MONOTONIC_RAW, & ts2);

		if (bcast_condition())
		{
			if (ts2.tv_sec - ts1.tv_sec >= KL)
			{
				if (write(sfd, & bcast_type, sizeof(int)) < 0)
					warn("write() broadcast failed");
				ts1 = ts2;
			} 
		}
		else 
			ts1 = ts2;
	}

	return NULL;
}

void* client_server_thread_func(void *p)
{
	pthread_detach(pthread_self());

	const int sfd = *((int*) p);
	free(p);

	int client_type = 0;

	// read client type
	if (read(sfd, & client_type, sizeof(int)) < 0) {
		warn("fail to receive client type");
		return NULL;
	}
		
	struct msg mesg;
	char data[MAX_MSG_LEN];
	mesg.data = & data[0];
	
	switch (client_type)
	{
		case 1:
			// recv mesg and push it into queue
			read(sfd, & mesg.T, sizeof(int));
			read(sfd, & mesg.len, sizeof(int));
			read(sfd, & mesg.data, mesg.len * sizeof(char));
			myqueue_push(& mesg);
			break;
		case 2:
			// pop a mesg from a queue and send it to client
			myqueue_front(& mesg);
			write(sfd, & mesg.T, sizeof(int));
			write(sfd, & mesg.len, sizeof(int));
			write(sfd, & mesg.data, mesg.len * sizeof(char));
			myqueue_pop();
			break;
		default:
			warn("invalid client type");
	}
	
	close(sfd);		

	return NULL;
}

int main()
{
	int N = 10;
	myqueue_init(N);

	int K, L;
	K = L = 5;

	int tcp_sfd = socket(AF_INET, SOCK_STREAM, 0);
	int udp_sfd = socket(AF_INET, SOCK_DGRAM, 0);

	int bcast_enable = 1;
	if (setsockopt(udp_sfd,	SOL_SOCKET, SO_BROADCAST, & bcast_enable, sizeof(bcast_enable)) < 0)
		err(EXIT_FAILURE, "setsockopt broadcast failed"); 

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(65535);	// listening on port 65535
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
	bta1.udp_sfd = bta2.udp_sfd = udp_sfd;
	pthread_t tid;
	pthread_create(& tid, NULL, bcast_thread_func, & bta1);
	pthread_create(& tid, NULL, bcast_thread_func, & bta2);

#warning Got some things TODO!

	// TODO: catch and handle signals 
	// sigterm, sigint - destroy que!, close fd's, and other clean up)

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
