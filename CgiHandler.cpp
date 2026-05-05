#include "CgiHandler.hpp"

CgiHandler::CgiHandler()
{
}

CgiHandler::CgiHandler(HttpRequest &request, const config::LocationConfig &location) //location info for cgi scripts
: _cgiPath(location.cgiPath), //rename later
_cgiExtention(location.cgiExtention), //rename later
_method(request.getMethod()),
_queryString("");
_body(request.getBody());
_contentType("");
_serverName("");
_headers(request.getHeaders());
{
	//build scriptpath (script_name from the config + path from the request object until '?')
	std::string	root = lc.root;
	if (root.end_with("/"))
		root.erase(root.size() - 1);
	std::string	raw = request._path;
	size_t	pos = raw.find('?'); //is the path already parsed in the httprequest?
	//if (URI found in path != std::string npos)
		//root = "/var/www/cgi" 
		_scriptPath = root + raw.substr(0, pos);
		_queryString = raw.substr(pos + 1);
	//else 
		//_scriptPath = root + raw;
	//set content type (parse request object for "content-type" and type is after that)
	_contentType = _headers.at("Content-Type");
	//set server name (parse request object for "host" and server name is after that)
	_serverName = _headers.at("Host");
}

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
		std::vector<std::string>	envs;
		envs.push_back("REQUEST_METHOD=" + _method);
		envs.push_back("QUERY_STRINGS=" + _queryString);
		if (request._contentLength)
			envs.push_back("CONTENT_LENGTH=" + std::to_string(request._contentLength));
		else
			envs.push_back("CONTENT_LENGTH=" + std::to_string(_body.size()));
		envs.push_back("SERVER_PROTOCOL=" + request._version);
		envs.push_back("SCRIPT_FILENAME=" + _scriptPath);
		envs.push_back("PATH=" + _cgiPath);
		envs.push_back("CONTENT_TYPE=" + _contentType);
		envs.push_back("SERVER_NAME=" + _serverName);
		envs.push_back("REDIRECT_STATUS=200");
		//now we push back all of the envs elements to a <char *> vector 
		std::vector<char *>	envp;
		std::vector<char *> args;
		args.push_back(const_cast<char *>(_cgiPath.cstr()));
		args.push_back(const_cast<char *>(_scriptPath.cstr()));
		args.push_back(NULL);
		execve(_cgiPath.cstr(), args.data(), envp.data()); // _path is cgiPath, argv consists of cgiPath, scriptpath and null 
		std::cerr << "CGI execve failed"
		_exit(1);
	}
	else
	{
		close(stdin_fd[0]);
		close(stdout_fd[1]);
		//here parent writes content of body to child, which is then read by child and appended to output, which is then returned
		//if method == POST and body is not empty and is chunked, create writing loop, write to stdin_fd[1]
		//if body is not chunked and we have a content length read file storing body  
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

	for (int i = 0; i < envp.size(); i++){
		delete envp[i];
	}
	delete[] envp;

	for (int i = 0; i < 3; i++){
		delete args[i];
	}
	delete[] args;
}
