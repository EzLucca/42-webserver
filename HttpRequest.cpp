#include <HttpRequest.hpp>


HttpRequest::HttpRequest()
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