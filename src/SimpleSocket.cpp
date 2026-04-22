#include "../include/SimpleSocket.hpp"

HDE::SimpleSocket::SimpleSocket(int domain, int service, int protocol, int port, ulong interface)
{
	// Define address structure
	address.sin_family = domain;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(interface);
	// Establish socket
	sock = socket(domain, service, protocol);
	test_connection(sock);
}

void	HDE::SimpleSocket::test_connection(int item_to_test)
{
	// Confirm that the socket or connection has been properly stablish
	if(item_to_test < 0)
	{
		perror("Failed to connect...");
		exit(EXIT_FAILURE);
	}
}

// Getters functions
struct sockaddr_in HDE::SimpleSocket::get_address()
{
	return address;
}
int HDE::SimpleSocket::get_sock()
{
	return sock;
}

// void	HDE::SimpleSocket::set_connection(int con)
// {
// 	connection = con;
// }
