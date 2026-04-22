#include "../include/BindingSocket.hpp"

// Constructors
HDE::BindingSocket::BindingSocket(int domain, int service, int protocol, int port, u_long interface) : SimpleSocket(domain, service, protocol, port, interface)
{
	connect_to_network(get_sock(), get_address());
	test_connection(binding);
}

// Definition of connect_to_network virtual function
void	HDE::BindingSocket::connect_to_network(int sock, struct sockaddr_in address)
{
	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	binding = bind(sock, (struct sockaddr *)&address, sizeof(address));
}

int HDE::BindingSocket::get_binding()
{
	return binding;
}
