#include "CgiHandler.hpp"

//****************************************************
 static void printconfig(ServerConfig config) // DEBUG:
 {
     // to print
     std::cout << config.getPort() << std::endl;
     std::cout << config.getHost() << std::endl;
     std::cout << config.getServerName() << std::endl;
     std::cout << config.getClientMaxBodySize() << std::endl;
     std::cout << config.getErrorPages() << std::endl;
     std::cout << std::endl;

     const auto& routes = config.getRoutes();

     for (const auto& [path, route] : routes) {
         std::cout << "Route path: " << path << "\n";
         std::cout << "Root: " << route.root << "\n";
         std::cout << "Index: " << route.index << "\n";
         std::cout << "Methods: ";
         for (const auto& m : route.allowedMethods)
             std::cout << m << " ";
         std::cout << "\nAutoIndex: " << route.autoIndex << "\n\n";
     }
 }
//****************************************************

CgiHandler::CgiHandler()
{
}

CgiHandler::CgiHandler(HttpRequest &request, ServerConfig &location) //location info for cgi scripts
:// _cgiPath(location.getRoute()), //cgiPass
//_cgiExtention(location.getCgiExtention()), //for validation
_method(request.getMethod()),
_queryString(""),
_contentType(""),
_serverName("")
{
	//SCRIPT PATH = ROOT FROM CONFIG + SCRIPT NAME FROM RAW URI(e.g. /cgi-bin/process.pl) + PATH_INFO FROM RAW URI BEFORE '?'
	_headers = request.getHeaders(); //TODO guard against malformed requests
	_bodyFilePath = request.getBodyFilePath();

	//***************************
	printconfig(location);
	//***************************
	
	const RouteConfig *cgiStruct = location.getRoute("/cgi-bin");
	std::string	root = cgiStruct->root;
	std::string	locationPath = cgiStruct->path;
	if (root.ends_with("/"))
		root.erase(root.size() - 1);
	std::string	raw = request.getUri();
	size_t	queryPos = raw.find('?');
	std::string	uriPath;
	if (queryPos != std::string::npos)
	{
		uriPath = raw.substr(0, queryPos);
		_queryString = raw.substr(queryPos + 1);
	}
	else
	{
		uriPath = raw;
		_queryString = "";
	}
	std::string	scriptName = uriPath.substr(locationPath.length());
	_scriptPath = root + scriptName;

	//***********************************
	std::cout << "root: " + root << std::endl;
	std::cout << "raw: " + raw << std::endl;
	std::cout << "scriptName: " + scriptName << std::endl;
	std::cout << "_scriptPath: " + _scriptPath << std::endl;
	//***********************************

	if  ((_method == "POST" && !_headers.count("content-type"))
		|| !_headers.count("host"))
	{
		std::cerr << "Malformed request\n";
	}
	_contentType = _headers.at("content-type");
	_serverName = _headers.at("host");

	//***********************************
	std::cout << "_serverName: " + _serverName << std::endl;
	std::cout << "_contentType: " + _contentType << std::endl;
	//***********************************
}

std::string	CgiHandler::cgiProcess(HttpRequest &request)
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
	else if (_pid == 0)
	{
		close(response_fd[0]);
		close(request_fd[1]);
		if (dup2(request_fd[0], STDIN_FILENO) < 0 || (dup2(response_fd[1], STDOUT_FILENO) < 0))
		{
			std::cerr << "CGI dup2 failed\n";
			_exit(1);
		}
		close(request_fd[0]);
		close(response_fd[1]);
//		std::vector<std::string>	_envs;
		_envs.push_back("REQUEST_METHOD=" + _method);
		_envs.push_back("QUERY_STRING=" + _queryString);
		if (request.getIsChunked()) //validate content_length to be under the maximum allowed
			_envs.push_back("CONTENT_LENGTH=" + std::to_string(request.getFullChunkBodySize()));
		else
			_envs.push_back("CONTENT_LENGTH=" + std::to_string(request.getContentLength()));
		_envs.push_back("SERVER_PROTOCOL=" + request.getVersion());
		_envs.push_back("SCRIPT_FILENAME=" + _scriptPath);
//		_envs.push_back("PATH_INFO=" + _cgiPath);
		_envs.push_back("CONTENT_TYPE=" + _contentType); //validation????????????????????
		_envs.push_back("SERVER_NAME=" + _serverName);
		_envs.push_back("REDIRECT_STATUS=200");//?????????????
		std::vector<char *>	_envp;
		for (auto &s : _envs)
			_envp.push_back(const_cast<char *>(s.c_str()));
		_envp.push_back(NULL);
		std::vector<char *> _args;
//		_args.push_back(const_cast<char *>(_cgiPath.cstr()));
		_args.push_back(const_cast<char *>(_scriptPath.c_str()));
		_args.push_back(NULL);
		execve(_scriptPath.c_str(), _args.data(), _envp.data()); // _path is cgiPath, argv consists of cgiPath, scriptpath and null 
		std::cerr << "CGI execve failed\n";
		_exit(1);
	}
	else
	{
		close(request_fd[0]);
		close(response_fd[1]);
		struct pollfd	fds[2];
		fds[0].fd = response_fd[0];
		fds[0].events = POLLIN;
		fds[0].revents = 0;

		fds[1].fd = request_fd[1];
		fds[1].events = POLLOUT;
		fds[1].revents = 0;
		int			status;
		std::string	responseOutput;
		char		responseBuf[4096];
		int	opennedBodyFile = -1;
		if (_method == "POST" && _bodyFilePath != "not-set")
		{
			opennedBodyFile = open(_bodyFilePath.c_str(), O_RDONLY);
			if (opennedBodyFile < 0)
			{
				close(request_fd[1]);
				close(response_fd[0]);
				std::cerr << "CGI failed to open body file\n";
				waitpid(_pid, &status, 0);
				return("");
			}
		}
		else
		{
			close(request_fd[1]);
			fds[1].fd = -1;
		}
		while (fds[0].fd != -1 || fds[1].fd != -1)
		{
			char	bodyBuf[4096];
			int	timeout = 5000;
			int	ready = poll(fds, 2, timeout);
			if (ready == -1)
			{
				close(request_fd[1]);
				close(response_fd[0]);
				if (opennedBodyFile != -1)
				{
					close(opennedBodyFile);
					opennedBodyFile = -1;
				}
				std::cerr << "Error in CGI poll()\n";
				waitpid(_pid, &status, WNOHANG); //replace with reliable child cleanup
				return("");
			}
			else if (ready == 0)
			{
				close(request_fd[1]);
				close(response_fd[0]);
				if (opennedBodyFile != -1)
				{
					close(opennedBodyFile);
					opennedBodyFile = -1;
				}
				std::cerr << "CGI poll timeout\n";
				waitpid(_pid, &status, WNOHANG);
				return("");
			}
			if (fds[1].revents & POLLOUT)
			{
				ssize_t	bytesRead = read(opennedBodyFile, bodyBuf, sizeof(bodyBuf));
				if (bytesRead == -1)
				{
					close(request_fd[1]);
					close(response_fd[0]);
					if (opennedBodyFile != -1)
					{
						close(opennedBodyFile);
						opennedBodyFile = -1;
					}
					std::cerr << "CGI body file read failed\n";
					waitpid(_pid, &status, WNOHANG); //replace with reliable child cleanup
					return("");
				}
				if (bytesRead == 0)
				{
					if (opennedBodyFile != -1)
					{
						close(opennedBodyFile);
						opennedBodyFile = -1;
					}
					close(request_fd[1]);
					fds[1].fd = -1;
					continue ;
				}
				ssize_t	totalWritten = 0;
				while (totalWritten < bytesRead)
				{
					ssize_t	bytesWritten = write(request_fd[1], bodyBuf + totalWritten, bytesRead - totalWritten);
					if (bytesWritten == -1)
					{
						close(request_fd[1]);
						close(response_fd[0]);
						if (opennedBodyFile != -1)
						{
							close(opennedBodyFile);
							opennedBodyFile = -1;
						}
						std::cerr << "CGI body file write to child failed\n";
						waitpid(_pid, &status, WNOHANG); //replace with reliable child cleanup
						return("");
					}
					totalWritten += bytesWritten;
				}
			}
			if (fds[0].revents & POLLIN)
			{
				ssize_t bytesRead = read(response_fd[0], responseBuf, sizeof(responseBuf)); //buf needs to be cleared every time
				if (bytesRead == -1)
				{
					close(response_fd[0]);
					std::cerr << "CGI response read failed\n";
					waitpid(_pid, &status, WNOHANG); //replace with reliable child cleanup
					return("");
				}
				if (bytesRead == 0)
				{
					close(response_fd[0]);
					fds[0].fd = -1;
					continue ;
				}
				responseOutput.append(responseBuf, bytesRead);
			}
		}
		if (fds[0].fd != -1)
			close(response_fd[0]);
		waitpid(_pid, &status, 0);
		if (!WIFEXITED(status))
			return ("");
		if (WEXITSTATUS(status) != 0) //check correct exit status
			return  ("");
		return(responseOutput);
	}
}

CgiHandler::~CgiHandler()
{
/*
	for (unsigned long i = 0; i < _envs.size(); i++){
		delete _envs[i];
	}
	delete[] _envs;

	for (unsigned long i = 0; i < _envp.size(); i++){
		delete _envp[i];
	}
	delete[] _envp;

	for (unsigned long i = 0; i < 3; i++){
		delete _args[i];
	}
	delete[] _args;*/
}
