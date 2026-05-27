# pragma once

#include <iostream>
#include <map>
#include <string>

//HttpRequest consist of request line, headers (body and query optional)
// Class holds the parsed URI, Method and headers
class Client;
class HttpRequest
{
    private:
            std::string                         _method;
            std::string                         _rawUri;
            std::string                         _uriPath;
            std::string                         _queryString;  
            std::string                         _version;
            std::string                         _filename;
            std::map<std::string, std::string>  _headers;
            std::string                         _location;
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
            void setQueryString(void);
            void setUriPath(std::string uriPath);
            void setFilename(std::string filename);
            void setLocation(std::string location);

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
            std::string                         getUriPath();
            std::string                         getFilename();
            std::string                         getQueryString();
            std::string                         getLocation();
            size_t                              getBytesWritten();
            bool                                getKeepAlive();

            void                                addBytesWritten(size_t bytes);
            void                                setupPathKeys(Client &activeClient);
        

};
