#ifndef HTTPPARSER_HPP
# define HTTPPARSER_HPP

#include <iostream>
#include <HttpRequest.hpp>
#include <Client.hpp>

// takes the raw string from client, parses it , and creates HttpRequest object
class HttpParser
{
    private:
            void parseRequestLine(std::string& line, HttpRequest& request);
            void parseHeaders(std::string& rawHeaders, HttpRequest& request);
            void parseChunkedBody(std::string& rawBody, HttpRequest& request);

    public:
            HttpParser();
            ~HttpParser();
            void parse(Client& client);




};

#endif