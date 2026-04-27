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
    // we have a order which we need to follow:
    // 1. we parse the request line, we are searching for \r\n and then we know its end of the request line
        //From here we get method, URI, and http version to our http request object
    if (client.getState() == READING_REQUESTLINE)
    {

    }
    
    // 2. we parse the headers, now we are searching for \r\n\r\n to know we have read the headers.
        //From here we parse all the headers, to our map
    if (client.getState() == READING_HEADERS)
    {

    }
    // 3. we parse the body, here we are comparing the content length number to the actual size of the vector. when the size == to the content length, we know thats end of the body
        //we just append all the bytes until we have appended the same amount the parsed contentlength value is. 
    if (client.getState() == READING_BODY)
    {
        
    }

    // REMEMBER TO CHANGE THE STATE in EVERYSTEP
}

