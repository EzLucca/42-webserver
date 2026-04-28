#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

#include <iostream>
#include <map>
#include <vector>

//HttpRequest consist of request line, headers (body and query optional)
// Class holds the parsed URI, Method and headers
class HttpRequest
{
    private:
            std::string _method;
            std::string _rawUri;       
            std::string _path;         
            std::string _queryString;  
            std::string _version;
            std::map<std::string, std::string> _headers;
            bool        _isChunked;
            size_t      _contentLength;
            std::vector<char> _body;
    public:
            HttpRequest();
            ~HttpRequest();
            void setMethod(std::string method);
            void setUri(std::string uri);
            void setVersion(std::string version);
            void setPath(std::string path);
            void setQueryString(std::string queryString);
            void setHeader(std::string key, std::string value);
            std::map<std::string, std::string> getHeaders() const; // Check that this works
        

};

#endif