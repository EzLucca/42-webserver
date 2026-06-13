# pragma once

#include <iostream>

#include "CgiHandler.hpp"

// Hold the status code and the final formatted body data
class Client ;
class HttpResponse
{

    private:
            int             _statusCode;
            std::string     _statusMessage;
			std::string	    _responseBody;
            std::string     _responseBuffer;
            int             _fileFd;
            bool            _isStreamingFile;


    public:
            HttpResponse();
            ~HttpResponse();


            void    setStreamingFlag(bool state);
            void    setStatusCode(int statusCode);
            void    setStatusMessage(std::string statusMessage);
			void	setResponseBody(std::string response);
            void    setResponseBuffer(std::string response);

            int             getStatusCode() const;
            std::string     getStatusMessage() const;
			std::string		getResponseBody() const;
			void			CgiReadResponse(CgiProcess &cgi, Client &activeClient);
            std::string     getMimeType(const std::string& filePath);
            void            buildRawResponse();
            std::string&    getBuffer();
            void            prepareFileStream(std::string filepath, Client& activeClient);
            int             getFileFd() const;
            bool            isStreaming() const;


};
