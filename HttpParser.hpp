#ifndef HTTPPARSER_HPP
# define HTTPPARSER_HPP

#include <iostream>
#include <HttpRequest.hpp>
#include <Client.hpp>
#include <ctype.h>

// takes the raw string from client, parses it , and creates HttpRequest object
class HttpParser
{
    private:
            void parseRequestLine(std::string& line, HttpRequest& request);
            void parseAllHeaders(std::string& rawHeaders, HttpRequest& request);
            void parseSingleHeader(std::string& line, HttpRequest& request);
            void parseChunkedBody(std::string& rawBody, HttpRequest& request);
            void parseBody(std::string& rawBody, HttpRequest& request);

    public:
            HttpParser();
            ~HttpParser();
            void parse(Client& client);
            std::string trimSpaces(std::string& value);
            std::string stringToLower(std::string value);





};

#endif