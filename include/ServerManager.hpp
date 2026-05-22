#pragma once

# include "ServerConfig.hpp"


// grand conductor, our waiter! all the poll logic will be here 
class ServerManager
{
    private:
        std::map<std::string, std::vector<ServerConfig>> _servers;
        // std::vector<ServerConfig> _servers2; // TODO: simplify the structures
        int _servercount;
    public:

        ServerManager() : _servercount(0) {};
        void    addServer(const ServerConfig & server);
        void    printServers() const;

        // getters
        std::string getServerLocation( 
                const std::string& serverName,
                const std::string& location,
                const std::string& locationKey) const;
        std::string getServerLocationMethods( 
                const std::string& serverName,
                const std::string& location,
                const std::string& locationKey,
                int method) const;
        std::string getServerValues( 
                const std::string& serverName,
                const std::string& key) const;
        std::string getServerErrorPages( 
                const std::string& serverName,
                int errorCode) const;
        int getServerCount() const;

        std::map<std::string, std::vector<ServerConfig>> getServers();
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
