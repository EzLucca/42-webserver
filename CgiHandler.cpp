#include "CgiHandler.hpp"

CgiHandler::CgiHandler(){
}

CgiHandler::CgiHandler(HttpRequest &request, const config::LocationConfig &location) //location info for cgi scripts
: _cgiShebang(location.cgiShebang),
_cgiExtention(location.cgiExtention),
_method(request.getMethod()),
_queryString("");
_body(request.getBody());
_contentType("");
_serverName("");
_headers(request.getHeaders());
{
	//build scriptpath
	//set querystring (everything after '?')
	//set content type (parse request object for "content-type" and type is after that)
	//set server name (parse request object for "host" and server name is after that)
}
/* nah
std::string	CgiHandler::createArgs(){

	args = new char*[2];
	args[0] = _scriptPath;
	args[1] = NULL;
}*/

std::string	CgiHandler::cgiProcess()
{
	int	stdin_fd[2];
	int	stdout_fd[2];

	if (pipe(stdin_fd) == -1 || pipe(stdout_fd) == -1)
	{
		std::cerr << "CGI pipe failed\n";
		return ("");
	}
	pid_t	_pid = fork();
	if (_pid == -1)
	{
		close(stdin_fd[0]);
		close(stdin_fd[1]);
		close(stdout_fd[0]);
		close(stdout_fd[1]);
		std::cerr << "CGI fork failed\n";
		return ("");
	}
	else if (pid == 0)
	{
		close(stdout_fd[0]);
		close(stdin_fd[1]);
		if (dup2(stdin_fd[0], STDIN_FILENO) < 0 || (dup2(stdout_fd[1], STDOUT_FILENO) < 0)
		{
			std::cerr << "CGI dup2 failed\n";
			_exit(1);
		}
		close(stdin_fd[0]);
		close(stdout_fd[1]);
		execve(_path, _args, _envp);
		std::cerr << "CGI execve failed"
		_exit(1);
	}
	else
	{
		close(stdin_fd[0]);
		close(stdout_fd[1]);
		//here parent writes content of body to child, which is then read by child and appended to output, which is then returned
		//if method == POST and body is not empty, create writing loop, write to stdin_fd[1]
		//afterwards, close stdin_fd[1]
		std::string	output;
		char		buf[4096];
		while (true)
		{
			ssize_t bytes = read(stdout_fd[0], buf.data(), sizeof(buf));
			if (bytes == -1)
			{
				close(stdout_fd[0]);
				exit(1);
			}
			if (bytes == 0)
				break ;
			output.append(buf, bytes);
		}
		close(stdout_fd[0]);
		waitpid(pid, &status, 0);
		if (WIFEXITED(status))
			return(WEXISTATUS(status));
		return(output);
	}
}

Cgihandler::~CgiHandler(){

	for (int i = 0; i < _envp.size(); i++){
		delete _envp[i];
	}
	delete[] _envp;

	for (int i = 0; i < 2; i++){
		delete _args[i];
	}
	delete[] args;
}
