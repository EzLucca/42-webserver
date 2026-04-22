#include "../include/testServer.hpp"
#include <fstream>
#include <sstream>

HDE::TestServer::TestServer() : SimpleServer(AF_INET, SOCK_STREAM, 0, 8080, INADDR_ANY, 10)
{
	launch();
}

void	HDE::TestServer::accepter()
{
	struct sockaddr_in address = get_socket()->get_address();
	int addrlen = sizeof(address);
	new_socket = accept(get_socket()->get_sock(), (struct sockaddr *)&address, (socklen_t *)&addrlen);
	read(new_socket, buffer, 30000);
}

void	HDE::TestServer::handler()
{
	std::cout << buffer << std::endl;
}

void HDE::TestServer::responder()
{
	std::ifstream file("src/index.html");
	if (!file.is_open())
	{
		const char* not_found =
			"HTTP/1.1 404 Not Found\r\n"
			"Content-Type: text/plain\r\n\r\n"
			"File not found";

		write(new_socket, not_found, strlen(not_found));
		close(new_socket);
		return;
	}

	std::stringstream buffer_stream;
	buffer_stream << file.rdbuf();
	std::string body = buffer_stream.str();

	std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: " + std::to_string(body.size()) + "\r\n"
		"Cache-Control: no-cache, no-store, must-revalidate\r\n"
		"Pragma: no-cache\r\n"
		"Expires: 0\r\n"
		"\r\n" +
		body;

	write(new_socket, response.c_str(), response.size());
	close(new_socket);
}

void	HDE::TestServer::launch()
{
	while(true)
	{
		accepter();
		handler();
		responder();
	}
}

HDE::TestServer::~TestServer() {}
