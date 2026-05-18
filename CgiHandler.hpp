#ifndef CGI_HPP
# define CGI_HPP

#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <vector>
#include <poll.h>
#include "HttpRequest.hpp"
#include "ServerConfig.hpp"

enum	CgiIoStatus
{
	CGI_IO_OK,
	CGI_IO_DONE,
	CGI_IO_ERROR
}

struct	CgiProcess
{
	bool		valid;
	pid_t		pid;
	int			requestFd;
	int			responseFd;
	int			bodyFileFd;
	std::string	output;
	bool		requestClosed;
	bool		responseClosed;
	time_t		startedAt;

	CgiProcess()
		: valid(false),
		pid(-1),
		requestFd(-1),
		responseFd(-1),
		bodyFileFd(-1),
		requestClosed(true),
		responseClosed(true)
		{
		}
};

// takes the httprequest object, sets up pipes, forks the process, executes the script
class CgiHandler
{
	private:
		std::string								_cgiPath; // where e.g. python interpreter lives e.g. usr/bin/python3
		std::string								_cgiExtention; // format to save the script
		std::string								_scriptPath; // path where the script lives
		std::string								_method; //request method
		std::string								_queryString;
		std::string								_bodyFilePath; //request body
		std::string								_contentType; //type determines what form to convert to
		std::map<std::string, std::string>		_headers; //std::map used to store key value pairs
		std::string								_serverName;
		std::vector<std::string>				_envs;
		std::vector<char *>						_envp;
		std::vector<char *>						_args;
		
		/* envp: query string, request method, content length, server protocol, 
		script filename, path info, content type, server name, redirect status */
	public:
		CgiHandler();
		CgiHandler(HttpRequest &request, ServerConfig &location);
		CgiProcess	CgiStart(HttpRequest &request);
		CgiIoStatus	CgiWriteToChild(CgiProcess &cgi);
		CgiIoStatus	CgiReadResponse(CgiProcess &cgi);
		~CgiHandler();
};

#endif

/*
 * CGI meta-variables
 *
 * Method -> REQUEST_METHOD (e.g. GET, POST)
 * Request target (path + optional ?query) -> split into:
 * 	path part used for routing (which script file to run),
 * 	query part (everything after ?, or empty)
 * Headers, minimum:
 * 	Content-Length (if body) - for CONTENT_LENGTH and bytes for child's stdin
 * 	Content-Type - CONTENT_TYPE for the CGI
 * 	Host - Often used to build authority / host for URLs
 * Body (for POST, etc.) - raw bytes to send on child's stdin after pipe setup
 * Connection context (may not live on HttpRequest alone): client IP -> REMOTE_ADDR, loval port -> SERVER_PORT, server name from config
 *
 * CGI Handler input: HttpRequest + config + maybe socket metadata
 *
 * Plan for CgiHandler
 *
 * 1. Decide script path + interpreter from URI + config
 * 2. Build the list of "NAME=value" strings (request + config + connection)
 * 3. Convert that list to envp for execve (std::vector<std::string>)
 * 4. Create pipes - stdout from child to parent; stdin from parent to child
 * 5. fork, child: dup2 stdout (and stdin for POST), close unused fds, execve script path with argv and envp
 * 6. Parent: write body to stdin pipe if needed, close write ends so child sees EOF, read child stdout into a buffer
 * 7. Parse CGI output: often headers until blank line, then body
 * 8. waitpid, map status to HTTP error
 */
