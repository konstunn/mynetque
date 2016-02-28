
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <limits.h> 
#include <unistd.h> 
#include <err.h> 
#include <string.h> 
#include "myqueue.h"

int main()
{
	int udp_sfd = socket(AF_INET, SOCK_DGRAM, 0);
	int tcp_sfd = socket(AF_INET, SOCK_STREAM, 0);

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
		struct sockaddr_in addr;
		socklen_t addrlen;

		// wait for udp notification
		// notice notification sender address (server address)
		if (recvfrom(udp_sfd, & client_type, sizeof(int), 0, (struct sockaddr*) & addr, & addrlen) < 0)
		{
			warn("recvfrom() failed");
			continue;
		} 
		warnx("notification received");

		addr.sin_port = htons(SERVER_PORT);

		if (client_type == 2)
		{
			// send the message to server
			if (connect(tcp_sfd, (struct sockaddr*) & addr, addrlen) < 0) {
				warn("connect() failed");
				continue;
			}

			int T; 
			int len; 
			unsigned char msg[MAX_MSG_LEN];

			read(tcp_sfd, & T, sizeof(int));
			read(tcp_sfd, & len, sizeof(int));
			read(tcp_sfd, & msg, len * sizeof(char));

			close(tcp_sfd);

			sleep(T);
		} else
			warnx("invalid client type");
	}

	return 0;
}
