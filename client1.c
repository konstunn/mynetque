
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
#include "message.pb-c.h"

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

			
			// generate message

			Message message;
			message__init(& message); 
			
			do 
				message.t = rand() % MAX_MSG_PROCTIME_SEC;
			while (message.t == 0);

			do 
				message.data.len = rand() % MAX_MSG_DATA_LEN;
			while (message.data.len == 0);

			uint8_t data[MAX_MSG_DATA_LEN+1];

			for (int i = 0; i < message.data.len; ++i) {
				do
					data[i] = rand() % UCHAR_MAX; 
				while (data[i] <= 32 || data[i] > 127);
			}
			message.data.data = data;

			// send the message to server
			
			if (connect(tcp_sfd, (struct sockaddr*) & addr, sizeof(struct sockaddr_in)) < 0) {
				warn("connect() failed");
				continue;
			}

			int msg_size = message__get_packed_size(& message);

			void *buf = malloc(msg_size);
			message__pack(& message, (uint8_t*) buf);

			write(tcp_sfd, & client_type, sizeof client_type);
			write(tcp_sfd, buf, msg_size);

			const int g_debug = 1;
			if (g_debug) {
				message.data.data[message.data.len] = '\0';	
				warnx("sent \"%s\", len = %lu, T = %d", message.data.data, message.data.len, message.t);
			}

			free(buf);

			close(tcp_sfd);
		} 
	}
	
	return 0;
}
