#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

#include <iostream>
#include <map>
#include <string>

//HttpRequest consist of request line, headers (body and query optional)
// Class holds the parsed URI, Method and headers
class HttpRequest
{
    private:
            std::string                         _method;
            std::string                         _rawUri;       
            std::string                         _queryString;  
            std::string                         _version;
            std::map<std::string, std::string>  _headers;
            bool                                _isChunked;
            size_t                              _contentLength;
            long                                _currentChunkSize;
            long                                _fullChunkBodySize;
            std::string                         _bodyFilePath;
            size_t                              _bytesWritten;
            bool                                _keepAlive;

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
            void setCurrentChunkSize(std::string chunkLine);
            void setFullChunkBodySize(size_t amount);
            void printHeaders();
            //void printBody();
            void setBodyFilePath(std::string setBodyFilePath);
            void setKeepAlive(bool keepAlive);

            //getters
            std::map<std::string, std::string>  getHeaders() const; // Check that this works
            size_t                              getContentLength();
            bool                                getIsChunked();
            long                                getCurrentChunkSize();
            long                                getFullChunkBodySize();
            std::string                         getMethod();
            std::string                         getUri();
            std::string                         getVersion();
            std::string                         getBodyFilePath();
            size_t                              getBytesWritten();
            bool                                getKeepAlive();

            void                                addBytesWritten(size_t bytes);
  
        

};

#endif
