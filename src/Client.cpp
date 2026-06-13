#include "Client.hpp"

Client::Client()
{
    _fd = -1;
    _state = READING_REQUESTLINE;
    _config = NULL;
    std::cout << "Client default constructor called." << std::endl;
}

Client::Client(int fd, const ServerConfig* config) :
    _fd(fd),
    _state(READING_REQUESTLINE),
    _config(config),
	_lastActivity(time(NULL))
{
    std::cout << "Client object created." << std::endl; 
}

Client::~Client()
{
    //if connection drops out, we delete tmp file/
	_request.cleanupBodyFile();
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

void	Client::updateLastActivity()
{
	_lastActivity = time(NULL);
}

time_t	Client::getLastActivity() const
{
	return (_lastActivity);
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
    _requestBuffer.erase(0, len);
    return ;
}

HttpRequest& Client::getRequest()
{
    return (_request);
}

HttpResponse& Client::getResponse()
{
    return (_response);
}

int    Client::getFd() const
{
    return (_fd);
}

const ServerConfig* Client::getConfig()
{
    return (_config);
}

void Client::resetForNextRequest() 
{
    this->_request = HttpRequest();   
    this->_response = HttpResponse(); 
    this->_state = READING_REQUESTLINE;
}
