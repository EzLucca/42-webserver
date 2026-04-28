#include <HttpRequest.hpp>


HttpRequest::HttpRequest() :
_currentChunkSize(-1)
{

}

HttpRequest::~HttpRequest()
{

}

void HttpRequest::setMethod(std::string method)
{
    _method = method;
}

void HttpRequest::setUri(std::string uri)
{
    _rawUri = uri;
}

void HttpRequest::setVersion(std::string version)
{
    _version = version;
}

void HttpRequest::setPath(std::string path)
{
    _path = path;
}

void HttpRequest::setQueryString(std::string queryString)
{
    _queryString = queryString;
}

std::map<std::string, std::string> HttpRequest::getHeaders() const
{
    return (_headers);
}

void HttpRequest::setHeader(std::string key, std::string value)
{
    _headers[key] = value;
}

void HttpRequest::setContentLength(std::string& value)
{
    _contentLength = std::stoi(value);
}

void HttpRequest::setIsChunked()
{
    _isChunked = true;
}

size_t HttpRequest::getContentLength()
{
    return (_contentLength);
}

bool   HttpRequest::getIsChunked()
{
    return (_isChunked);
}

long    HttpRequest::getCurrentChunkSize()
{
    return (_currentChunkSize);
}

void    HttpRequest::setBody(std::string body)
{
    _body = body;
}

void    HttpRequest::setCurrentChunkSize(std::string chunkLine)
{
    _currentChunkSize = std::stoi(chunkLine, 0, 16);
}

void HttpRequest::appendToBody(std::string bodydata)
{
    _body.append(bodydata);
}