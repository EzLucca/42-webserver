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

        /* envp: query string, request method, content length, server protocol,
           script filename, path info, content type, server name, redirect status */
    public:
        CgiHandler();
        // CgiHandler(HttpRequest &request, ServerConfig &server);
        // CgiHandler(Client &activeClient, ServerConfig &server);
        CgiHandler(Client &activeClient);
        CgiProcess CgiStart(HttpRequest &request);
        //		CgiIoStatus	CgiWriteToChild(CgiProcess &cgi);
        //		void		setStatus(CgiIoStatus status);
        //		CgiIoStatus	getStatus();
        ~CgiHandler();
};

/*
 * CGI meta-variables
 *
 * Method -> REQUEST_METHOD (e.g. GET, POST)
 * Request target (path + optional ?query) -> split into:
 * 	path part used for routing (which script file to run),
 * 	query part (everything after ?, or empty)
 * Headers, minimum:
 * 	Content-Length (if body) - for CONTENT_LENGTH and bytes for child's
 * stdin Content-Type - CONTENT_TYPE for the CGI Host - Often used to build
 * authority / host for URLs Body (for POST, etc.) - raw bytes to send on
 * child's stdin after pipe setup Connection context (may not live on
 * HttpRequest alone): client IP -> REMOTE_ADDR, loval port -> SERVER_PORT,
 * server name from config
 *
 * CGI Handler input: HttpRequest + config + maybe socket metadata
 *
 * Plan for CgiHandler
 *
 * 1. Decide script path + interpreter from URI + config
 * 2. Build the list of "NAME=value" strings (request + config + connection)
 * 3. Convert that list to envp for execve (std::vector<std::string>)
 * 4. Create pipes - stdout from child to parent; stdin from parent to child
 * 5. fork, child: dup2 stdout (and stdin for POST), close unused fds, execve
 * script path with argv and envp
 * 6. Parent: write body to stdin pipe if needed, close write ends so child sees
 * EOF, read child stdout into a buffer
 * 7. Parse CGI output: often headers until blank line, then body
 * 8. waitpid, map status to HTTP error
 */
