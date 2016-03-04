
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h> 
#include <limits.h> 
#include <unistd.h> 
#include <err.h> 
#include <string.h> 
#include "myqueue.h"
#include "message.pb-c.h"

int main()
{
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
		int client_type = 0;
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(struct sockaddr_in);

		// wait for udp notification
		// notice notification sender address (server address)
		if (recvfrom(udp_sfd, & client_type, sizeof(int), 0, (struct sockaddr*) & addr, & addrlen) < 0)
		{
			warn("recvfrom() failed");
			continue;
		} 

		addr.sin_port = htons(SERVER_PORT);

		if (client_type == 2)
		{
			int tcp_sfd = socket(AF_INET, SOCK_STREAM, 0);

			// send the message to server
			if (connect(tcp_sfd, (struct sockaddr*) & addr, addrlen) < 0) {
				warn("connect() failed");
				continue;
			}

			uint8_t *buf = (uint8_t*) malloc(MAX_MSG_SIZE);

			Message *message;

			write(tcp_sfd, & client_type, sizeof client_type);

			int msg_len = read(tcp_sfd, buf, MAX_MSG_SIZE);
			message = message__unpack(NULL, msg_len, buf);
			
			const int g_debug = 1;
			
			if (g_debug) {
				memcpy(buf, message->data.data, message->data.len);
				buf[message->data.len] = '\0'; 
				warnx("rcvd \"%s\", len = %lu, T = %d", buf, message->data.len, message->t);
			}

			close(tcp_sfd);

			sleep(message->t);

			free(buf);
			message__free_unpacked(message, NULL);
		} 
	}

	return 0;
}
