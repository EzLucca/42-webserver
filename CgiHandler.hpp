#ifndef CGIHANDLER_HPP
# define CGIHANDLER_HPP

#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


// takes the httprequest object, sets up pipes, forks the process, executes the script
class CgiHandler
{
	private:
		int				_fd[2];
		pid_t				_pid;
		int				_status;
		size_t				_ret;
		std::vector<char>		_buf;
		std::string			_path;
		std::string			_args;
		std::string			_body;
		size_t				_contentLength;
		std::vector<std::string>	_envp; // query string, request method, http headers *, gateway interface, content *, server/client metadata
		int				cgiProcess();
	public:
		CgiHandler();
		CgiHandler(char **envp, std::string &body, std::string path);
		std::string	createArgs();
		std::string	getCmd();
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
 * Connection context (mey not live on HttpRequest alone): client IP -> REMOTE_ADDR, loval port -> SERVER_PORT, server name from config
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
