#pragma once

#include "SimpleSocket.hpp"

namespace HDE
{
	class ConnectingSocket : public SimpleSocket {
		private:
			int	connecting;
		public:
			// Constructors
			ConnectingSocket(int domain, int service, int protocol, int port, ulong interface);
			// Virtual function from parent
			void	connect_to_network(int sock, struct sockaddr_in address);
	};
}
