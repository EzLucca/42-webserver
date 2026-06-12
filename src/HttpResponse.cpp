#include "HttpResponse.hpp"
#include "Client.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <string>

HttpResponse::HttpResponse() : 
    _statusCode(200),
    _statusMessage("OK"),
    _fileFd(-1),
    _isStreamingFile(false)
{
    std::cout << "HttpResponse constructor called." << std::endl;
}

int HttpResponse::getStatusCode() const
{
    return (_statusCode);
}

std::string HttpResponse::getStatusMessage() const
{
    return (_statusMessage);
}

std::string	HttpResponse::getResponseBody() const
{
    return (_responseBody);
}

void    HttpResponse::setStatusCode(int statusCode)
{
    _statusCode = statusCode;
}

void    HttpResponse::setStatusMessage(std::string statusMessage)
{
    _statusMessage = statusMessage;
}

void	HttpResponse::setResponseBody(std::string response)
{
    _responseBody = response;
}

void	HttpResponse::CgiReadResponse(CgiProcess &cgi, Client &activeClient)
{
    int			status;
    char		responseBuf[4096];

    //  try to read from pipe
    ssize_t		bytesRead = read(cgi.responseFd, responseBuf, sizeof(responseBuf));

    std::cout << "------------->>>>>>>> bytesRead: " << bytesRead << std::endl;
    // error or blockin
    if (bytesRead == -1)
    {
        std::cout << "This is bytes -1" << std::endl;
        // (If no data is ready yet, just keep waiting)
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            activeClient.setState(CGI_IO_OK);
            return ;
        }
        // real read error, close everything and kill the process
        close(cgi.responseFd);
        cgi.responseFd = -1;
        cgi.responseClosed = true;
        cgi.valid = false;
        std::cerr << "CGI response read failed\n";
        waitpid(cgi.pid, &status, WNOHANG);
        activeClient.setState(CGI_IO_ERROR);
        return ;
    }

    // data received
    if (bytesRead > 0)
    {
        std::cout << "bytes > 0" << std::endl;

        // append read data to output string
        cgi.output.append(responseBuf, bytesRead);
        activeClient.setState(CGI_IO_OK);
        return ; 
    }


    if (bytesRead == 0)
    {
        // (Wait for child blocking, because we know it has closed the pipe)
        pid_t result = waitpid(cgi.pid, &status, WNOHANG);
        if (result == 0)
		{
			activeClient.setState(CGI_IO_OK);
			return ;
		}
        if (result == -1)
        {
            activeClient.setState(CGI_IO_ERROR);
            return ;
        }
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            activeClient.setState(CGI_IO_ERROR);
            return ;
        }
        // (Everything succeeded, tell main.cpp we are done!)
        activeClient.setState(CGI_IO_DONE);
        return ;
    }


    activeClient.setState(CGI_IO_ERROR);
    return ;
}

HttpResponse::~HttpResponse()
{
}

std::string HttpResponse::getMimeType(const std::string& filePath)
{
    // findt the last '.' dot in the filename
    size_t dotPos = filePath.find_last_of('.');
    
    
    // if no extension, return default binary data type
    if (dotPos == std::string::npos)
        return "application/octet-stream"; 

    // extract extension
    std::string extension = filePath.substr(dotPos);

    // comapre to most common types
    if (extension == ".html" || extension == ".htm") return "text/html";
    if (extension == ".css") return "text/css";
    if (extension == ".js") return "application/javascript";
    if (extension == ".png") return "image/png";
    if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    if (extension == ".gif") return "image/gif";
    if (extension == ".ico") return "image/x-icon";
    if (extension == ".txt") return "text/plain";
    if (extension == ".json") return "application/json";
    if (extension == ".pdf") return "application/pdf";

    // uknown extension
    return "application/octet-stream";
}

void HttpResponse::buildRawResponse()
{
    std::string finalString = "HTTP/1.1 " + std::to_string(_statusCode) + " " + _statusMessage + " \r\n";
    finalString += "Content-Length: " + std::to_string(_responseBody.size()) + "\r\n";
    finalString += "\r\n"; // The mandatory blank line
    finalString += _responseBody;

    _responseBuffer = finalString;
}

std::string& HttpResponse::getBuffer()
{
    return (_responseBuffer);
}

void HttpResponse::prepareFileStream(std::string filepath, Client& activeClient)
{
    //  open file
    _fileFd = open(filepath.c_str(), O_RDONLY);

    if (_fileFd < 0) // < 0 means it failed to open
    {
        _statusCode = 404;
        _statusMessage = "Not Found";
        filepath = activeClient.getConfig()->getErrorPage(404);
        _fileFd = open(filepath.c_str(), O_RDONLY);
        
        if (_fileFd < 0) {
            this->_responseBuffer = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
            this->_isStreamingFile = false;
            return;
        }
    }

    // get filesize with stat
    struct stat stat_buf;
    fstat(_fileFd, &stat_buf);
    size_t fileSize = stat_buf.st_size;

    // build headers first
    std::string contentType = getMimeType(filepath); 
    
    std::string headers = "HTTP/1.1 " + std::to_string(_statusCode) + " " + _statusMessage + "\r\n";
    headers += "Content-Type: " + contentType + "\r\n";
    headers += "Content-Length: " + std::to_string(fileSize) + "\r\n";
    headers += "Connection: keep-alive\r\n\r\n";
    
    this->_responseBuffer = headers;
    this->_isStreamingFile = true;
}

int     HttpResponse::getFileFd() const
{
    return (_fileFd);
}

bool    HttpResponse::isStreaming() const
{
    return (_isStreamingFile);
}

void    HttpResponse::setStreamingFlag(bool state)
{
    _isStreamingFile = state;
}

void	HttpResponse::setResponseBuffer(std::string response)
{
    _responseBuffer = response;
}
