#include <Client.hpp>

Client::Client(int fd) :
_fd(fd),
_state(READING_HEADERS)
{
    std::cout << "Client object created." << std::endl; 
}

Client::~Client()
{
    std::cout << "Client object destroyed." << std::endl; 
}

void Client::appendToBuffer(const char* data, ssize_t size)
{
    _requestBuffer.append(data, size);
}         

std::string Client::getBuffer()
{
    return (_requestBuffer);
}