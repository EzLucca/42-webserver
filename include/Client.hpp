# pragma once

#include <iostream>
#include <vector>
#include <ctime>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"

//these are possible states (these can change still)
enum ClientState {
    READING_REQUESTLINE,
    READING_HEADERS,        // POST Master is waiting for \r\n\r\n
    READING_BODY,
    READING_BODY_CHUNKED,   // POST Master is reading chunks/data
    PARSING_REQUEST_LINE,   // Parsing request line
    PARSING_HEADERS,        // Parsing headers
    PARSING_BODY,           // Parsing body
    PROCESSING,
    CGI_CALL,             // GET Master is matching routing rules / opening files
	CGI_IO_OK,
	CGI_IO_DONE,
	CGI_IO_ERROR,
    WAITING_FOR_CGI,        // CGI Master is waiting for the pipe to have data
    WRITING_RESPONSE,       // Sending the formatted data back to the browser
    ERROR,
    FINISHED                // Flag to tell the main loop to close the socket
};

class   HttpResponse;
class   HttpRequest;
// Holds the connection state, the raw string, and owns its specific request/response objects
class   Client 
{
    // What do we need to store in client object? 
    private:
            int                     _fd;            // Client socket
            ClientState             _state;         // Store the client state
            
            std::string             _requestBuffer; //where we append the request

            HttpRequest             _request;
            HttpResponse            _response;
            const ServerConfig*     _config;
			time_t					_lastActivity;

    public:
            Client();
            Client(int fd, const ServerConfig* config); // constructor sets state = Reading headears on default
            ~Client();

            void	setState(ClientState state);
            void	setConfig(ServerConfig config);
            void	appendToBuffer(const char* data, ssize_t size);
            const	std::string getBuffer() const;
            void	eraseFromBuffer(size_t length);
			void	updateLastActivity();


            HttpRequest& getRequest();
            HttpResponse& getResponse();
            ClientState getState() const;
            const ServerConfig* getConfig();
            int getFd() const;
			time_t	getLastActivity();
};
