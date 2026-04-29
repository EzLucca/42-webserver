#include "CgiHandler.hpp"

CgiHandler::CgiHandler(){
}

CgiHandler::CgiHandler(std::string method, std::string path, std::string queryString, std::string body, size_t contentLength) : _method(method), _path(path), _queryString(queryString), _body(body), _contentLength(contentLength){
}

int	CgiHandler::cgiProcess() {
	if (pipe(_fd) == -1)
		exit(1);
	_pid = fork();
	if (_pid == -1) {
		close(fd[0]);
		close(fd[1]);
		exit(1);
	}
	else if (pid == 0) {
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		execve(_path, _args, _envp);
		_exit(1);
	}
	else {
		close(fd[1]);
		dup2(fd[0], STDIN_FILENO);
		while (_ret > 0) {
			_ret = read(fd[0], _buf.data(), CONTENT_LENGTH);
			if (_ret == -1){
				close(fd[0]);
				exit(1);
			}
			std::cout.write(buf.data(), _ret); // change to write to client later
		}
		close(fd[0]);
		waitpid(pid, &status, 0);
		if (WIFEXITED(status))
			return(WEXISTATUS(status));
		return(1);
	}
}

Cgihandler::~CgiHandler(){
}


