
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <limits.h> 
#include <unistd.h> 
#include <err.h> 
#include "myqueue.h"

int main()
{
	int udp_sfd = socket(AF_INET, SOCK_DGRAM, 0);
	int tcp_sfd = socket(AF_INET, SOCK_STREAM, 0);
	
	while (1)
	{
		int buf;		
		struct sockaddr_in addr;
		socklen_t addrlen;

		// wait for udp notification
		// notice notification sender address (server address)
		if (recvfrom(udp_sfd, & buf, sizeof(int), 0, (struct sockaddr*) & addr, & addrlen) < 0)
		{
			warn("recvfrom() failed");
			continue;
		} 
		warnx("notification received");

		addr.sin_port = htons(SERVER_PORT);

		if (buf == 1) 
		{
			warnx("generating message..");
			// generate a message
			unsigned char msg[MAX_MSG_LEN];
			
			int T = rand() % MAX_MSG_PROCTIME_SEC;
			int len = rand() % MAX_MSG_LEN;

			for (int i = 0; i < len; ++i)
				msg[i] = rand() % UCHAR_MAX;

			// send the message to server
			if (connect(tcp_sfd, (struct sockaddr*) & addr, addrlen) < 0) {
				warn("connect() failed");
				continue;
			}

			write(tcp_sfd, & T, sizeof(int));
			write(tcp_sfd, & len, sizeof(int));
			write(tcp_sfd, & msg, len * sizeof(char));

			close(tcp_sfd);
		}
	}
	
	return 0;
}
