#include "CgiHandler.hpp"

CgiHandler::CgiHandler(){
}

CgiHandler::CgiHandler(char **envp, std::string &body, std::string path) : envp(_envp), _body(&body), _path(path){

}

std::string	CgiHandler::createArgs(){

	args = new char*[2];
	args[0] = //script path
	args[1] = NULL;
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

	for (int i = 0; i < _envp.size(); i++){
		delete _envp[i];
	}
	delete[] _envp;

	for (int i = 0; i < 2; i++){
		delete _args[i];
	}
	delete[] args;
}
