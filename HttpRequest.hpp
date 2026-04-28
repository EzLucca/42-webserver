#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

#include <iostream>
#include <map>
#include <vector>
#include <string>

//HttpRequest consist of request line, headers (body and query optional)
// Class holds the parsed URI, Method and headers
class HttpRequest
{
    private:
            std::string                         _method;
            std::string                         _rawUri;       
            std::string                         _path;         
            std::string                         _queryString;  
            std::string                         _version;
            std::map<std::string, std::string>  _headers;
            bool                                _isChunked;
            size_t                              _contentLength;
            std::string                         _body;
            long                                _currentChunkSize;
    public:
            HttpRequest();
            ~HttpRequest();

            //setters
            void setMethod(std::string method);
            void setUri(std::string uri);
            void setVersion(std::string version);
            void setPath(std::string path);
            void setQueryString(std::string queryString);
            void setHeader(std::string key, std::string value);
            void setContentLength(std::string& value);
            void setIsChunked();
            void setBody(std::string body);
            void setCurrentChunkSize(std::string chunkLine);
            void appendToBody(std::string bodydata);

            //getters
            std::map<std::string, std::string>  getHeaders() const; // Check that this works
            size_t                              getContentLength();
            bool                                getIsChunked();
            long                                getCurrentChunkSize();
            std::string                         getMethod();
            std::string                         getUri();
            std::string                         getVersion();
        

};

#endif