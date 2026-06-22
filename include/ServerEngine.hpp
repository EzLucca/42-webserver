#pragma once

# include <map>
# include <poll.h>
# include <ctime>
# include <sys/wait.h>

# include "HttpException.hpp"
# include "Client.hpp"
# include "CgiHandler.hpp"
# include "HttpParser.hpp"
# include "helperUtils.hpp"

# define MAX_FDS 100
# define POLL_TIMEOUT_MS 1000
# define CLIENT_TIMEOUT 30
# define CGI_TIMEOUT 10

class ServerEngine
{
    private:
        // const ServerManager& _manager;
        std::map<int, Client> _clients;             // clientFd -> Client
        std::map<int, CgiProcess> _cgiProcesses;    // cgi responseFd -> CgiProcess
        std::map<int, int> _fdRegistry;             // cgi responseFd -> clientFd
        std::map<int, const ServerConfig*> _masterSocketRegistry;
        struct pollfd _fds[MAX_FDS];
        HttpParser _httpParser;

        void printFdRegistry() const;
        bool isClientWaitingForCgi(int clientFd) const;
        void removeFdFromPoll(int fd);
        void checkClientTimeouts(time_t now);
        void checkCgiTimeouts(time_t now);

        void acceptNewClient(int triggered_fd);
        void handleCgiFd(int i, int triggered_fd);
        void handleClientFd(int i, int currentFd);

    public:
        ServerEngine(struct pollfd fds[],
                     std::map<int, const ServerConfig*>& masterSocketRegistry);

        void    run();

};
