
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <limits.h> 
#include <unistd.h> 
#include <err.h> 
#include <string.h> 
#include <time.h> 
#include "myqueue.h"

int main()
{
	srand(time(NULL));

	int udp_sfd = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in addr;
	memset(& addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT); 
	addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);	

	const int so_reuseaddr = 1;
	if ((setsockopt(udp_sfd, SOL_SOCKET, SO_REUSEADDR, & so_reuseaddr, sizeof(int))) < 0)
		err(EXIT_FAILURE, "setsockopt() failed"); 
			
	if (bind(udp_sfd, (struct sockaddr*) & addr, sizeof(struct sockaddr_in)) < 0)
		err(EXIT_FAILURE, "bind() failed");
	
	while (1)
	{
		int client_type;
		socklen_t addrlen = sizeof(struct sockaddr_in);

		// wait for udp notification
		// get notification sender address (server address)
		if (recvfrom(udp_sfd, & client_type, sizeof(int), 0, (struct sockaddr*) & addr, & addrlen) < 0)
		{
			warn("recvfrom() failed");
			continue;
		} 

		addr.sin_port = htons(SERVER_PORT);

		if (client_type == 1)
		{
			int tcp_sfd = socket(AF_INET, SOCK_STREAM, 0);
			
			// generate a message
			unsigned char msg[MAX_MSG_LEN+1];
			
			int T;
			do 
				T = rand() % MAX_MSG_PROCTIME_SEC;
			while (T == 0);

			int len;
			do 
				len = rand() % MAX_MSG_LEN;
			while (len == 0);

			for (int i = 0; i < len; ++i) {
				do {
					msg[i] = rand() % UCHAR_MAX; 
				} while (msg[i] <= 32 || msg[i] > 127);
			}

			// send the message to server
			if (connect(tcp_sfd, (struct sockaddr*) & addr, sizeof(struct sockaddr_in)) < 0) {
				warn("connect() failed");
				continue;
			}

			write(tcp_sfd, & client_type, sizeof client_type);
			write(tcp_sfd, & T, sizeof(int));
			write(tcp_sfd, & len, sizeof(int));
			write(tcp_sfd, & msg, len * sizeof(char));

			const int g_debug = 1;
			if (g_debug) {
				msg[len] = '\0';
				warnx("sent \"%s\", len = %d, T = %d", msg, len, T);
			}

			close(tcp_sfd);

		} 
	}
	
	return 0;
}
