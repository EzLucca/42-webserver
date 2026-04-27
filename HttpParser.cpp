#include <HttpParser.hpp>

void HttpParser::parseRequestLine(std::string& line, HttpRequest& request)
{

}

void HttpParser::parseHeaders(std::string& rawHeaders, HttpRequest& request)
{

}

void HttpParser::parseChunkedBody(std::string& rawBody, HttpRequest& request)
{

}

HttpParser::HttpParser()
{
    std::cout << "HttParser constructor called." << std::endl;
}

HttpParser::~HttpParser()
{
    std::cout << "HttParser destructor called." << std::endl;
}

void HttpParser::parse(Client& client)
{
    //main logic in the parse

    //we land here after we have read the whatever client was sending
    // we have a order which we need to 
}