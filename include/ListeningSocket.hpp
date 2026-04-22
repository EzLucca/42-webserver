#pragma once

#include "BindingSocket.hpp"

namespace HDE
{
	class ListeningSocket : public BindingSocket {
		private:
			int backlog;
			int listening;
		public:
			// Constructors
			ListeningSocket(int domain, int service, int protocol, int port, ulong interface, int bklg);
			void	start_listening();

			int	get_listening();
			int	get_backlog();
	};
}
