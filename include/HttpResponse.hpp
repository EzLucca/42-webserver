# pragma once

#include <iostream>
#include "CgiHandler.hpp"

// Hold the status code and the final formatted body data
class Client ;
class HttpResponse
{
    //Once the server processes request, it send back and HTTP response with following components

    //Status Line -- Contains HTTP version, status code and status message
    private:
            int         _statusCode;
            std::string _statusMessage;
			std::string	_responseBody;

    // HEADERS, Metadata about the response, such as content type and caching policies
    public:
            HttpResponse();
            ~HttpResponse();

            //setters
            void    setStatusCode(int statusCode);
            void    setStatusMessage(std::string statusMessage);
			void	setResponseBody(std::string response);
            //getters
            int             getStatusCode() const;
            std::string     getStatusMessage() const;
			std::string		getResponseBody() const;
			void			CgiReadResponse(CgiProcess &cgi, Client &activeClient);
            std::string     getMimeType(const std::string& filePath);
    // BODY -- The actual content returned , such as HTML, JSON, or text

};
