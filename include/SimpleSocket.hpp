#pragma once

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>

namespace HDE
{
	class SimpleSocket {
		private:
			struct sockaddr_in address;
			int sock;
			// int connection;

		public:
			// Constructor
			SimpleSocket(int domain, int service, int protocol, int port, ulong interface);
			// Virtual function to connct to a network
			virtual void	connect_to_network(int sock, struct sockaddr_in address) = 0;
			// Function to test sockets and connections
			void	test_connection(int);
			// Getter functions
			struct sockaddr_in get_address();
			int get_sock();
			// void	set_connection(int con);
	};
}
