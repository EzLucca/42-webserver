#pragma once

# include "ServerConfig.hpp"
# include <map>
# include <vector>
# include <poll.h>

// grand conductor, our waiter! all the poll logic will be here 
class ServerManager
{
    private:
        std::vector<ServerConfig> _servers;
        // std::vector<ServerConfig> _servers2; // TODO: simplify the structures
        int _servercount;
        std::map<int, const ServerConfig*> _masterSocketRegistry;

    public:

        ServerManager() : _servercount(0) {};
        void    addServer(const ServerConfig & server);
        void    printServers() const;

        int    getServerCount() const;


        void cleanExitServers();
        const ServerConfig* getServerByFd(int fd);
        std::vector<ServerConfig> getServers();
        std::vector<int> getMasterFds() const;

        bool setupMasterSockets(struct pollfd fds[], std::map<int, const ServerConfig*>& masterSocketRegistry);
        const std::map<int, const ServerConfig*>& getMasterSocketRegistry() const;
        void shutDownServers();
};
