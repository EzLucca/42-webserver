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

const std::string HttpResponse::getStatusMessage() const
{
    return (_statusMessage);
}

void    HttpResponse::setStatusCode(int statusCode)
{
    _statusCode = statusCode;
}

void    HttpResponse::setStatusMessage(std::string statusMessage)
{
    _statusMessage = statusMessage;
}

HttpResponse::~HttpResponse()
{
    std::cout << "HttpResponse destructor called." << std::endl;
}
