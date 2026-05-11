#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

#include <iostream>

// Hold the status code and the final formatted body data
class HttpResponse
{
    //Once the server processes request, it send back and HTTP response with following components

    //Status Line -- Contains HTTP version, status code and status message
    private:
            int         _statusCode;
            std::string _statusMessage;

    // HEADERS, Metadata about the response, such as content type and caching policies
    public:
            HttpResponse();
            ~HttpResponse();

            //setters
            void    setStatusCode(int statusCode);
            void    setStatusMessage(std::string statusMessage);
            //getters
            int             getStatusCode() const;
            std::string     getStatusMessage() const;  
    // BODY -- The actual content returned , such as HTML, JSON, or text

};

#endif