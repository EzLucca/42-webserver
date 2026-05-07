#include "CgiHandler.hpp"

CgiHandler::CgiHandler()
{
}

CgiHandler::CgiHandler(HttpRequest &request, const config::LocationConfig &location) //location info for cgi scripts
: _cgiPath(location.getCgiPath()), //rename later
_cgiExtention(location.getCgiExtention()), //rename later
_method(request.getMethod()),
_queryString("");
_contentType("");
_serverName("");
{
	//SCRIPT PATH = ROOT FROM CONFIG + SCRIPT NAME FROM RAW URI(e.g. /cgi-bin/process.pl) + PATH_INFO FROM RAW URI BEFORE '?'
	_headers(request.getHeaders()); //TODO guard against malformed requests
	_bodyFilePath(request.getBodyFilePath());
	std::string	root = lc.getRoot;
	if (root.end_with("/"))
		root.erase(root.size() - 1);
	std::string	raw = request.getUri();
	size_t	pos = raw.find('?');
	if (root.find(uri) != std::string npos)
		root = "/var/www/cgi";
	if (pos != std::string npos)
	{
		_scriptPath = root + raw.substr(0, pos);
		_queryString = raw.substr(pos + 1);
	}
	else
	{
		_scriptPath = root + raw;
		_quesryString = "";
	}
	if  (!_headers.count("content-type") || !_headers.count("host"))
	{
		std::cerr << "Malformed request\n";
		delete _scriptPath;
		delete _queryString;
		_exit(1);
	}
	_contentType = _headers.at("content-type");
	_serverName = _headers.at("host");1
}

std::string	CgiHandler::cgiProcess()
{
	int	request_fd[2];
	int	response_fd[2];

	if (pipe(request_fd) == -1 || pipe(response_fd) == -1)
	{
		std::cerr << "CGI pipe failed\n";
		return ("");
	}
	pid_t	_pid = fork();
	if (_pid == -1)
	{
		close(request_fd[0]);
		close(request_fd[1]);
		close(response_fd[0]);
		close(response_fd[1]);
		std::cerr << "CGI fork failed\n";
		return ("");
	}
	else if (pid == 0)
	{
		close(response_fd[0]);
		close(request_fd[1]);
		if (dup2(request_fd[0], STDIN_FILENO) < 0 || (dup2(response_fd[1], STDOUT_FILENO) < 0)
		{
			std::cerr << "CGI dup2 failed\n";
			_exit(1);
		}
		close(request_fd[0]);
		close(response_fd[1]);
		std::vector<std::string>	envs;
		envs.push_back("REQUEST_METHOD=" + _method);
		envs.push_back("QUERY_STRINGS=" + _queryString);
		if (request._isChunked)
			envs.push_back("CONTENT_LENGTH=" + std::to_string(request.getFullChunkBodySize()));
		else
			envs.push_back("CONTENT_LENGTH=" + std::to_string(request.getContentLength());
		envs.push_back("SERVER_PROTOCOL=" + request._version);
		envs.push_back("SCRIPT_FILENAME=" + _scriptPath);
		envs.push_back("BODY_PATH=" + _bodyFilePath);
		envs.push_back("PATH_INFO=" + _cgiPath);
		envs.push_back("CONTENT_TYPE=" + _contentType);
		envs.push_back("SERVER_NAME=" + _serverName);
		envs.push_back("REDIRECT_STATUS=200");
		std::vector<char *>	envp;
		int	i = 0;
		while (envs[i])
		{
			envp.push_back(const_cast<char *>(envs[i]))
			i++;
		}
		envp[i] = NULL;
		std::vector<char *> args;
		args.push_back(const_cast<char *>(_cgiPath.cstr()));
		args.push_back(const_cast<char *>(_scriptPath.cstr()));
		args.push_back(NULL);
		execve(_cgiPath.cstr(), args.data(), envp.data()); // _path is cgiPath, argv consists of cgiPath, scriptpath and null 
		std::cerr << "CGI execve failed\n";
		_exit(1);
	}
	else
	{
		close(request_fd[0]);
		close(response_fd[1]);
		ssize_t	bytesWritten = write(request_fd[1], _body.cstr(), _body.size())); // maybe useless???????
		if (bytesWritten == -1)
		{
			close(request_fd[1]);
			close(response_fd[0]);
			exit(1);
		}
		close(request_fd[1]);
		std::string	responseOutput;
		char		buf[4096];
		while (true)
		{
			ssize_t bytesRead = read(response_fd[0], buf, sizeof(buf)); //buf needs to be cleared every time
			if (bytesRead == -1)
			{
				close(response_fd[0]);
				exit(1);
			}
			if (bytesRead == 0)
				break ;
			responseOutput.append(buf, bytesRead);
		}
		close(response_fd[0]);
		waitpid(pid, &status, 0);
		if (WIFEXITED(status))
			return(WEXISTATUS(status));
		return(responseOutput);
	}
}

Cgihandler::~CgiHandler(){

	for (int i = 0; i < envs.size(); i++){
		delete envs[i];
	}
	delete[] envs;

	for (int i = 0; i < envp.size(); i++){
		delete envp[i];
	}
	delete[] envp;

	for (int i = 0; i < 3; i++){
		delete args[i];
	}
	delete[] args;
}
