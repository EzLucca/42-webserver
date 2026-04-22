#pragma once

#include "SimpleSocket.hpp"

namespace HDE
{
	class BindingSocket : public SimpleSocket {
		private:
			int	binding;
		public:
			// Constructors
			BindingSocket(int domain, int service, int protocol, int port, ulong interface);
			// Virtual function from parent
			void	connect_to_network(int sock, struct sockaddr_in address);
			int get_binding();
	};
}
