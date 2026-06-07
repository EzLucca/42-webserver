#include "HttpResponse.hpp"
#include "Client.hpp"

HttpResponse::HttpResponse() : 
    _statusCode(200),
    _statusMessage("not-set")
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
