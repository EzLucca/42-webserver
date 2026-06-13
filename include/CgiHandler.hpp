#pragma once

#include "HttpRequest.hpp"
#include "ServerConfig.hpp"
#include "ServerManager.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

struct CgiProcess {
    bool valid;
    pid_t pid;
    int responseFd;
    int bodyFileFd;
    std::string output;
    bool responseClosed;
    time_t startedAt;

    CgiProcess()
        : valid(false), pid(-1), responseFd(-1), bodyFileFd(-1),
        responseClosed(true) {}
};

// takes the httprequest object, sets up pipes, forks the process, executes the
// script
class Client;
class CgiHandler {
    private:
        std::string                         _root;
        std::string                         _cgiPass; // where e.g. python interpreter lives e.g. usr/bin/python3
        std::string                         _cgiExtention; // format to save the script
        std::string                         _scriptPath;   // path where the script lives
        std::string                         _method;       // request method
        std::string                         _queryString;
        std::string                         _bodyFilePath; // request body
        std::string                         _contentType;  // type determines what form to convert to
        std::map<std::string, std::string>  _headers; // std::map used to store key value pairs
        std::string                         _serverName;
        std::vector<std::string>            _envs;
        std::vector<char *>                 _envp;
        std::vector<char *>                 _args;

    public:
        CgiHandler();
        CgiHandler(Client &activeClient);
        CgiProcess CgiStart(HttpRequest &request);
        ~CgiHandler();
};
