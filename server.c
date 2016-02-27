
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
		if (bcast_condition())
		{
			clock_gettime(CLOCK_MONOTONIC_RAW, &ts2);

			if (ts2.tv_sec - ts1.tv_sec == KL)
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

int main()
{
	int N = 10;
	myqueue_init(N);

	int K, L;
	K = L = 5;

	int tcp_sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int udp_sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

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

	// TODO: catch and handle signals (sigterm - destroy que!, close fd's?)
	// may be should fork()

	int cfd;
	while (1) // working loop
	{
		cfd = accept(tcp_sfd, NULL, NULL);
		if (cfd == -1) {
			warn("accept() failed");
			continue;
		}

		int client_type;

		// read client type
		read(cfd, & client_type, sizeof(int));
		
		switch (client_type)
		{
			case 1:
				// TODO
				break;
			case 2:
				// TODO
				break;
			default:
				warn("invalid client type");
				close(cfd);
				continue;
		}
		
		// TODO: create corresponding thread to process accepted connection
		// here	
	}

	myqueue_destroy();
	
	return 0; 
}
