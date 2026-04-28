#include "Client.hpp"

Client::Client() :
_fd(-1)
{
    std::cout << "Client default constructor called." << std::endl;
}

Client::Client(int fd) :
_fd(fd),
_state(READING_REQUESTLINE)
{
    std::cout << "Client object created." << std::endl; 
}

Client::~Client()
{
    std::cout << "Client object destroyed." << std::endl; 
}

ClientState Client::getState() const
{
    return (_state);
}
void Client::setState(ClientState state)
{
    _state = state;
    return ;
}

void Client::appendToBuffer(const char* data, ssize_t size)
{
    _requestBuffer.append(data, size);
}         

const std::string Client::getBuffer() const
{
    return (_requestBuffer);
}

void Client::eraseFromBuffer(size_t len)
{
    _requestBuffer.erase(len, 0);
    return ;
}

HttpRequest& Client::getRequest()
{
    return (_request);
}

