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
 	ssize_t		bytesRead = read(cgi.responseFd, responseBuf, sizeof(responseBuf));
 	if (bytesRead == -1)
 	{
        std::cout << "It is stuck here1" << std::endl;
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			activeClient.setState(CGI_IO_OK);
			return ;
		}
 		close(cgi.responseFd);
		cgi.responseFd = -1;
		cgi.responseClosed = true;
		cgi.valid = false;
 		std::cerr << "CGI response read failed\n";
 		waitpid(cgi.pid, &status, WNOHANG);
 		activeClient.setState(CGI_IO_ERROR);
		return ;
 	}
	if (bytesRead > 0)
	{
        std::cout << "It is stuck here2" << std::endl;
		cgi.output.append(responseBuf, bytesRead);
		activeClient.setState(CGI_IO_OK);
		return ; 
	}
 	if (bytesRead == 0)
 	{
        std::cout << "It is stuck here3" << std::endl;
 		close(cgi.responseFd);
 		cgi.responseFd = -1;
		cgi.responseClosed = true;
		pid_t	result = waitpid(cgi.pid, &status, WNOHANG);
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
		activeClient.setState(CGI_IO_DONE);
		return ;
 	}
	activeClient.setState(CGI_IO_ERROR);
	return ;
}

HttpResponse::~HttpResponse()
{
}
