# pragma once

#include <iostream>
#include <ctype.h>
#include <fstream>

#include "HttpRequest.hpp"
#include "Client.hpp"

// takes the raw string from client, parses it , and creates HttpRequest object
class HttpParser
{
    private:
            void parseRequestLine(std::string& line, HttpRequest& request);
            void parseAllHeaders(std::string& rawHeaders, Client& client);
            void parseSingleHeader(std::string& line, Client& client);
            void parseChunkedBody(std::string& rawBody, HttpRequest& request);
            void parseBodyIntoFile(int clientFd, std::string& bodyData, HttpRequest& request);
            void validateHeaders(HttpRequest& request);

    public:
            HttpParser();
            ~HttpParser();
            void parse(Client& client);
            std::string trimSpaces(std::string& value);
            std::string stringToLower(std::string value);
            





};
