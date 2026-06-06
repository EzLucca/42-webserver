#include "CgiHandler.hpp"
#include "Client.hpp"
#include "helperUtils.hpp"

//****************************************************
// static void printconfig(ServerConfig config) // DEBUG:
// {
//     // to print
//     std::cout << config.getPort() << std::endl;
//     std::cout << config.getHost() << std::endl;
//     std::cout << config.getServerName() << std::endl;
//     std::cout << config.getClientMaxBodySize() << std::endl;
//     std::cout << config.getErrorPages() << std::endl;
//     std::cout << std::endl;
//
//     const auto& routes = config.getRoutes();
//
//     for (const auto& [path, route] : routes) {
//         std::cout << "Route path: " << path << "\n";
//         std::cout << "Root: " << route.root << "\n";
//         std::cout << "Index: " << route.index << "\n";
//         std::cout << "Methods: ";
//         for (const auto& m : route.allowedMethods)
//             std::cout << m << " ";
//         std::cout << "\nAutoIndex: " << route.autoIndex << "\n\n";
//     }
// }
//****************************************************

CgiHandler::CgiHandler()
{
}

// CgiHandler::CgiHandler(HttpRequest &request, ServerConfig &server) //location info for cgi scripts
CgiHandler::CgiHandler(Client &activeClient) //location info for cgi scripts
    : _cgiPass(""), //cgiPass
    _method(activeClient.getRequest().getMethod()),
    // _queryString(activeClient.getRequest().getQueryString()),
    _queryString(""),
    _contentType(""),
    _serverName("")
{
    //SCRIPT PATH = ROOT FROM CONFIG + SCRIPT NAME FROM RAW URI(e.g. /cgi-bin/process.pl) + PATH_INFO FROM RAW URI BEFORE '?'
    _headers = activeClient.getRequest().getHeaders(); //TODO guard against malformed requests
    _bodyFilePath = activeClient.getRequest().getBodyFilePath();
    _queryString = activeClient.getRequest().getQueryString();

    //***************************
    // printconfig(location);
    //***************************

    const RouteConfig *config;
    config = activeClient.getConfig()->getRoute(activeClient.getRequest().getLocationKey());
    std::unordered_map<std::string, std::vector<std::string>> victorRoute;
    victorRoute = config->vectorRoute;
    std::unordered_map<std::string, std::vector<std::string>>::const_iterator it;
    for (it = victorRoute.begin(); it != victorRoute.end(); it++)
    {
        if (it->first == "root")
            _root = it->second[0];
        if (it->first == "cgi_pass")
            _cgiPass = it->second[0];
    }

    std::cout << _root << " #######3" << std::endl;
    std::cout << _cgiPass << " ########" << std::endl;
    // std::string::const_iterator it = config->vectorRoute.find("root");
    // if (it != config->vectorRoute.end() && !it->second.empty()) {
    //     root = it->second[0];
    //     std::cout << " #######4" << std::endl;
    // }
    //
    // auto shiit = config->vectorRoute.find("cgi_pass");
    // if (shiit != config->vectorRoute.end() && !shiit->second.empty()) {
    //     _cgiPass = shiit->second[0];
    // }
    std::string checkHijack = activeClient.getRequest().getFilename();
    //checking if the hijackmode is activated 
    if (endsWith(checkHijack, ".py") == true)
    {
        _scriptPath = checkHijack;
    }
    else
    {
        _scriptPath = _root + activeClient.getRequest().getUriPath();  
    }
    validatePath(_scriptPath);
    std::cout << _scriptPath << " ########62384623" << std::endl;

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

CgiProcess	CgiHandler::CgiStart(HttpRequest &request)
{
    int	response_fd[2];
    // int	status;
    CgiProcess	cgi;

    cgi.startedAt = time(NULL); //to track timeout in main
    if (pipe(response_fd) == -1)
    {
        std::cerr << "CGI pipe failed\n";
        return (cgi);
    }
    pid_t	_pid = fork();
    if (_pid == -1)
    {
        close(response_fd[0]);
        close(response_fd[1]);
        std::cerr << "CGI fork failed\n";
        return (cgi);
    }
    else if (_pid == 0)
    {
        close(response_fd[0]);
        cgi.bodyFileFd = open(_bodyFilePath.c_str(), O_RDONLY);
        if (cgi.bodyFileFd < 0)
        {
            close(cgi.responseFd);
            cgi.responseFd = -1;
            cgi.responseClosed = true;
            cgi.valid = false;
            std::cerr << "CGI failed to open body file\n";
            return (cgi);
        }
        if (dup2(cgi.bodyFileFd, STDIN_FILENO) < 0 || (dup2(response_fd[1], STDOUT_FILENO) < 0))
        {
            std::cerr << "CGI dup2 failed\n";
            _exit(1);
        }
        close(response_fd[1]);
        // close(cgi.bodyFileFd);
        //		std::vector<std::string>	_envs;
        _envs.push_back("REQUEST_METHOD=" + _method);
        _envs.push_back("QUERY_STRING=" + _queryString);
        if (request.getIsChunked()) //validate content_length to be under the maximum allowed
            _envs.push_back("CONTENT_LENGTH=" + std::to_string(request.getFullChunkBodySize()));
        else
            _envs.push_back("CONTENT_LENGTH=" + std::to_string(request.getContentLength()));
        _envs.push_back("SERVER_PROTOCOL=" + request.getVersion());
        _envs.push_back("SCRIPT_FILENAME=" + _scriptPath);
        _envs.push_back("PATH_INFO=" + _cgiPass);
        _envs.push_back("UPLOAD_DIR=" + _root);
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
        execve(_args[0], _args.data(), _envp.data()); // _path is cgiPath, argv consists of cgiPath, scriptpath and null 
        std::cout << "It is stuck here4 from child" << std::endl;
        std::cerr << "CGI execve failed\n";
        _exit(1);
    }
    else
    {
        std::cout << "This is the parent inside cgihandler" << std::endl;
        close(response_fd[1]);
        // close(cgi.bodyFileFd);
        fcntl(response_fd[0], F_SETFL, O_NONBLOCK);
        cgi.valid = true;
        cgi.pid = _pid;
        cgi.responseFd = response_fd[0];
        cgi.bodyFileFd = -1;
        cgi.output = "";
        cgi.responseClosed = false;
        return (cgi);
    }
}

/*
   struct pollfd	fds[2];
   fds[0].fd = response_fd[0];
   fds[0].events = POLLIN;
   fds[0].revents = 0;

   fds[1].fd = request_fd[1];
   fds[1].events = POLLOUT;
   fds[1].revents = 0;

   CgiIoStatus	CgiHandler::CgiWriteToChild(CgiProcess &cgi)
   {
   int			status;
   char		bodyBuf[4096];
   ssize_t	bytesRead = read(cgi.bodyFileFd, bodyBuf, sizeof(bodyBuf));
   if (bytesRead == -1)
   {
   if (errno == EAGAIN || errno == EWOULDBLOCK)
   return (CGI_IO_OK);
   close(cgi.requestFd);
   close(cgi.responseFd);
   cgi.requestFd = -1;
   cgi.responseFd = -1;
   cgi.requestClosed = true;
   cgi.responseClosed = true;
   cgi.valid = false;
   if (cgi.bodyFileFd != -1)
   {
   close(cgi.bodyFileFd);
   cgi.bodyFileFd = -1;
   }
   std::cerr << "CGI body file read failed\n";
   waitpid(cgi.pid, &status, WNOHANG);
   return(CGI_IO_ERROR);
   }
   if (bytesRead == 0)
   {
   if (cgi.bodyFileFd != -1)
   {
   close(cgi.bodyFileFd);
   cgi.bodyFileFd = -1;
   }
   close(cgi.requestFd);
   cgi.requestFd = -1;
   cgi.requestClosed = true;
   return (CGI_IO_DONE);
   }
   ssize_t	totalWritten = 0;
   while (totalWritten < bytesRead)
   {
   ssize_t	bytesWritten = write(cgi.requestFd, bodyBuf + totalWritten, bytesRead - totalWritten);
   if (bytesWritten == -1)
   {
   if (errno == EAGAIN || errno == EWOULDBLOCK)
   return (CGI_IO_OK);
   close(cgi.requestFd);
   close(cgi.responseFd);
   cgi.requestFd = -1;
   cgi.responseFd = -1;
   cgi.responseClosed = true;
   cgi.requestClosed = true;
   cgi.valid = false;
   if (cgi.bodyFileFd != -1)
   {
   close(cgi.bodyFileFd);
   cgi.bodyFileFd = -1;
   }
   std::cerr << "CGI body file write to child failed\n";
   waitpid(cgi.pid, &status, WNOHANG);
   return(CGI_IO_ERROR);
   }
totalWritten += bytesWritten;
}
return (CGI_IO_OK);
}*/

// void	CgiHandler::setStatus(CgiIoStatus status)
// {
//     _cgiStatus = status;
// }
//
// CgiIoStatus	CgiHandler::getStatus()
// {
//     return (_cgiStatus);
// }
//
CgiHandler::~CgiHandler()
{
}
