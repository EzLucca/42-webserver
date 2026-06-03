#include "HttpResponse.hpp"
#include "Client.hpp"

HttpResponse::HttpResponse() : 
    _statusCode(0),
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
        pid_t result = waitpid(cgi.pid, &status, 0);

        // (DO NOT CLOSE FD HERE! main.cpp will handle it.)
        cgi.responseClosed = true;

        // checkin if child process crashed 
        std::cout << "bytes = 0" << std::endl;
        if (result == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
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
