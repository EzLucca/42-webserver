#include "HttpResponse.hpp"

HttpResponse::HttpResponse() : 
_statusCode(0),
_statusMessage("not-set")
{
    std::cout << "HttpResponse constructor called." << std::endl;
}

int HttpResponse::getStatusCode() const
{
    return (_statusCode);
}

std::string HttpResponse::getStatusMessage() const
{
    return (_statusMessage);
}

std::string	HttpResponse::getResponseBody() const
{
	return (_responseBody);
}

void    HttpResponse::setStatusCode(int statusCode)
{
    _statusCode = statusCode;
}

void    HttpResponse::setStatusMessage(std::string statusMessage)
{
    _statusMessage = statusMessage;
}

void	HttpResponse::setResponseBody(std::string response)
{
	_responseBody = response;
}

HttpResponse::~HttpResponse()
{
}
