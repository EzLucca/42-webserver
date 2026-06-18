#include "CgiHandler.hpp"
#include "Client.hpp"
#include "helperUtils.hpp"

CgiHandler::CgiHandler()
{
}

CgiHandler::CgiHandler(Client &activeClient)
    : _cgiPass(""),
    _method(activeClient.getRequest().getMethod()),
    _queryString(""),
    _contentType(""),
    _serverName("")
{
    //SCRIPT PATH = ROOT FROM CONFIG + SCRIPT NAME FROM RAW URI(e.g. /cgi-bin/process.pl) + PATH_INFO FROM RAW URI BEFORE '?'
    _headers = activeClient.getRequest().getHeaders();
    _bodyFilePath = activeClient.getRequest().getBodyFilePath();
    _queryString = activeClient.getRequest().getQueryString();

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


    std::string checkHijack = activeClient.getRequest().getFilename();
    if (checkHijack == "var/cgi/cgi-bin/betterUpload.py")
    {
        _scriptPath = checkHijack;
    }
    else
    {
        _scriptPath = _root + activeClient.getRequest().getUriPath();  
    }
    validatePath(_scriptPath);

    //  Safe extraction for Content-Type
    std::map<std::string, std::string>::const_iterator ct_it = _headers.find("content-type");
    if (ct_it != _headers.end()) {
        _contentType = ct_it->second;
    } else {
        _contentType = ""; // Empty string if the header doesn't exist (normal for GET)
    }

    //  Safe extraction for Host
    std::map<std::string, std::string>::const_iterator host_it = _headers.find("host");
    if (host_it != _headers.end()) {
        _serverName = host_it->second;
    } else {
        _serverName = ""; // Or handle as a fatal error since Host is mandatory in HTTP/1.1
    }
}

CgiProcess	CgiHandler::CgiStart(HttpRequest &request)
{
    int	response_fd[2];
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
        //  Close the read-end of the pipe (the child only writes to it)
        close(response_fd[0]);
        
        //  Setup STDIN --- ONLY for POST requests!
        if (_method == "POST")
        {
            cgi.bodyFileFd = open(_bodyFilePath.c_str(), O_RDONLY);
            if (cgi.bodyFileFd < 0)
            {
                std::cerr << "CGI failed to open body file\n";
                _exit(1); // CRITICAL: Instantly kill the child if it fails.
            }
            
            if (dup2(cgi.bodyFileFd, STDIN_FILENO) < 0)
            {
                std::cerr << "CGI dup2 STDIN failed\n";
                _exit(1);
            }
            close(cgi.bodyFileFd); // Close the original fd now that it is dup'd
        }

        //  Setup STDOUT --- ALWAYS do this for EVERY request!
        if (dup2(response_fd[1], STDOUT_FILENO) < 0)
        {
            std::cerr << "CGI dup2 STDOUT failed\n";
            _exit(1);
        }
        //  Now that STDOUT is securely hooked up to the pipe, close the original pipe FD
        close(response_fd[1]);
        _envs.push_back("REQUEST_METHOD=" + _method);
        _envs.push_back("QUERY_STRING=" + _queryString);
        if (request.getIsChunked())
            _envs.push_back("CONTENT_LENGTH=" + std::to_string(request.getFullChunkBodySize()));
        else
            _envs.push_back("CONTENT_LENGTH=" + std::to_string(request.getContentLength()));
        _envs.push_back("SERVER_PROTOCOL=" + request.getVersion());
        _envs.push_back("SCRIPT_FILENAME=" + _scriptPath);
        _envs.push_back("PATH_INFO=" + _cgiPass);
        _envs.push_back("UPLOAD_DIR=" + _root);
        _envs.push_back("CONTENT_TYPE=" + _contentType);
        _envs.push_back("SERVER_NAME=" + _serverName);
        _envs.push_back("REDIRECT_STATUS=200");
        std::vector<char *>	_envp;
        for (auto &s : _envs)
            _envp.push_back(const_cast<char *>(s.c_str()));
        _envp.push_back(NULL);
        std::vector<char *> _args;
        _args.push_back(const_cast<char *>(_scriptPath.c_str()));
        _args.push_back(NULL);
        execve(_args[0], _args.data(), _envp.data());
        std::cerr << "CGI execve failed\n";
        _exit(1);
    }
    else
    {
        std::cout << "This is the parent inside cgihandler" << std::endl;
        close(response_fd[1]);

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

CgiHandler::~CgiHandler()
{
}
