#pragma once

# include "ServerConfig.hpp"


// grand conductor, our waiter! all the poll logic will be here 
class ServerManager
{
    private:
        std::vector<ServerConfig> _servers;
        // std::vector<ServerConfig> _servers2; // TODO: simplify the structures
        int _servercount;
        // probably all registries will lie here
        // fd as key and port as value

    public:

        ServerManager() : _servercount(0) {};
        void    addServer(const ServerConfig & server);
        void    printServers() const;

        int    getServerCount() const;


        //void setupNetwork(); // TODO
        //ServerConfig* getServerByFd(int fd); //TODO THIS

        std::vector<ServerConfig> getServers();
        // TEST: version 3
        // const ServerConfig* findServer(
        //     const std::string& host,
        //     int port) const;
        //
        // const LocationConfig* findLocation(
        //     const ServerConfig& server,
        //     const std::string& uri) const;
        // ~TEST:
};
