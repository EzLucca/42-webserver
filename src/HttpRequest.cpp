#include "HttpRequest.hpp"
#include "Client.hpp"
#include "HttpException.hpp"
#include "helperUtils.hpp"


HttpRequest::HttpRequest() :
    _isChunked(false),
    _contentLength(0),
    _currentChunkSize(-1),
    _fullChunkBodySize(0),
    _bodyFilePath("not-set"),
    _bytesWritten(0),
    _keepAlive(true)
{
    std::cout << "HttpRequest default constructor called" << std::endl;
}

HttpRequest::~HttpRequest()
{
    std::cout << "HttpRequest destructor called" << std::endl;
}

void HttpRequest::setMethod(std::string method)
{
    _method = method;
}

void HttpRequest::setUri(std::string uri)
{
    _rawUri = uri;
}

void HttpRequest::setVersion(std::string version)
{
    _version = version;
}

void HttpRequest::setQueryString(std::string queryString)
{
    _queryString = queryString;
}

std::map<std::string, std::string> HttpRequest::getHeaders() const
{
    return (_headers);
}

void HttpRequest::setHeader(std::string key, std::string value)
{
    _headers[key] = value;
}

void HttpRequest::setContentLength(std::string& value)
{
    int parsedLength = std::stoi(value);

    // We need to check manually minus values
    if (parsedLength < 0) {
        throw std::invalid_argument("Negative Content-Length"); 
    }
    _contentLength = static_cast<size_t>(parsedLength);
}

void HttpRequest::setIsChunked()
{
    _isChunked = true;
}

size_t HttpRequest::getContentLength()
{
    return (_contentLength);
}

bool   HttpRequest::getIsChunked()
{
    return (_isChunked);
}

void	HttpRequest::resetCurrentChunkSize()
{
	_currentChunkSize = -1;
}

long    HttpRequest::getCurrentChunkSize()
{
    return (_currentChunkSize);
}

void    HttpRequest::setCurrentChunkSize(std::string chunkLine)
{
	try
	{
		_currentChunkSize = std::stoi(chunkLine, 0, 16);
	}
	catch (const std::invalid_argument& e)
	{
		throw HttpException(400, "Bad Request: Invalid chunk size");
	}
	catch (const std::out_of_range& e)
	{
		throw HttpException(400, "Bad Request: Chunk size too large");
	}
	if (_currentChunkSize < 0)
		throw HttpException(400, "Bad Request: Invalid chunk size");
}

std::string HttpRequest::getMethod()
{
    return (_method);
}

std::string HttpRequest::getUri()
{
    return (_rawUri);
}
std::string HttpRequest::getVersion()
{
    return (_version);
}

void HttpRequest::printHeaders()
{
    //create iterator, we use const because we are not gonna change anything
    std::map<std::string, std::string>::const_iterator it;
    std::cout << "Printing Headers!" << std::endl;


    for (it = _headers.begin(); it != _headers.end(); ++it) 
    {
        //first is header (key)
        //second is the value

        std::cout << "Header: [" << it->first << "] " 
            << "Value: [" << it->second << "]" << std::endl;
    }
}

/*
   void HttpRequest::printBody()
   {
   std::cout << "Printing the body!" << std::endl;
   std::cout << _body << std::endl;
   }
   */
void HttpRequest::setFullChunkBodySize(size_t amount)
{
    _fullChunkBodySize += amount;
}

void HttpRequest::setBodyFilePath(std::string bodyFilePath)
{
    _bodyFilePath = bodyFilePath;
}

std::string HttpRequest::getBodyFilePath()
{
    return (_bodyFilePath);
}

long HttpRequest::getFullChunkBodySize()
{
    return (_fullChunkBodySize);
}

size_t HttpRequest::getBytesWritten()
{
    return (_bytesWritten);
}

void HttpRequest::addBytesWritten(size_t bytes)
{
    _bytesWritten += bytes;
}

void HttpRequest::setKeepAlive(bool keepAlive)
{
    _keepAlive = keepAlive;
}

bool HttpRequest::getKeepAlive()
{
    return (_keepAlive);
}

void HttpRequest::setQueryString(void)
{
    std::string uriRequest = getUri();
    size_t	queryPos = uriRequest.find('?');
    if (queryPos != std::string::npos)
    {
        _uriPath = uriRequest.substr(0, queryPos);
        _queryString = uriRequest.substr(queryPos + 1);
    }
    else
    {
        _uriPath = uriRequest;
        _queryString = "";
    }
    //uriPath = /cgi-bin/file.py  
    // size_t pos = klist.find(uriPath)
    // if(pos != std::string::npos)
    // {
    //         size_t finalpos;
    //         if(pos > finalpos)
    //             finalpos = pos;
    // }
    // uriPath = raw.substr(0, finalpos);
}

void HttpRequest::setUriPath(std::string uriPath)
{
    _uriPath = uriPath;
}

void HttpRequest::setFilename(std::string filename)
{
    _filename = filename;
}

std::string HttpRequest::getUriPath()
{
    return (_uriPath);
}

std::string HttpRequest::getFilename()
{
    return (_filename);
}

std::string HttpRequest::getQueryString()
{
    return (_queryString);
}

void HttpRequest::setLocation(std::string location)
{
    _locationKey = location;
}
std::string HttpRequest::getLocationKey()
{
    return (_locationKey);
}

void	HttpRequest::cleanupBodyFile()
{
	if (_bodyFilePath != "not-set")
	{
		std::remove(_bodyFilePath.c_str());
		_bodyFilePath = "not-set";
	}
}

void    HttpRequest::setupPathKeys(Client &activeClient)
{
    std::string uriRequest = activeClient.getRequest().getUri();
    std::cout << "Requested URI: " << uriRequest << std::endl;

    // 2. Grab the direct pointer to the rulebook!
    const ServerConfig *activeConfig = activeClient.getConfig();

    std::vector<std::string> klist = activeConfig->getLocationList();
    // activeClient.getRequest().setQueryString();
    setQueryString();

    std::cout << getUriPath() << std::endl;
    std::cout << getQueryString() << std::endl;

    std::string objstring = getUriPath();
    size_t finalpos = 0;
    for (const std::string &s : klist) {
        size_t pos = getUriPath().find(s);
        if(pos != std::string::npos)
        {
            pos = s.size();
            if(pos > finalpos)
                finalpos = pos;
            std::cout << finalpos << std::endl;
        }

    }
    setLocation(objstring.substr(0, finalpos));
    setFilename(objstring.substr(finalpos));

    if (getFilename() != "")
    {
        setFilename(objstring.substr(finalpos + 1));
    }

    std::cout << getUriPath() << std::endl;
    std::cout << getLocationKey() << std::endl;
    std::cout << getFilename() << std::endl;
}


void HttpRequest::handleDeleteRequest(Client& activeClient)
{

    HttpRequest& request = activeClient.getRequest();
    HttpResponse& response = activeClient.getResponse();
    
    std::string targetPath = buildSafeTargetPath(activeClient.getConfig()->getRoute(request.getLocationKey()), request.getFilename());
    // check that  the file exists
    if (access(targetPath.c_str(), F_OK) != 0) 
    {
        response.setStatusCode(404);
        response.setResponseBody("<html><body>404 Not Found: File does not exist</body></html>");
        return;
    }

    // check write permissions, to check we can delete the file
    if (access(targetPath.c_str(), W_OK) != 0) 
    {
        response.setStatusCode(403);
        response.setResponseBody("<html><body>403 Forbidden: Permission denied</body></html>");
        return;
    }

    // try to delete the file
    if (remove(targetPath.c_str()) == 0) 
    {
        // 204 No Content is standard if you don't send a body, 
        // 200 OK is standard if you send a success message.
        response.setStatusCode(200); 
        response.setResponseBody("<html><body>200 OK: File deleted successfully</body></html>");
    } 
    else 
    {
        // error, couldnt delete
        response.setStatusCode(500);
        response.setResponseBody("<html><body>500 Internal Server Error: Could not delete file</body></html>");
    }
}

