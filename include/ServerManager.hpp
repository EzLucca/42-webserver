#pragma once

# include "ServerConfig.hpp"

// grand conductor, our waiter! all the poll logic will be here 
class ServerManager
{
    private:
        std::map<std::string, std::vector<ServerConfig>> _servers;
        // std::vector<ServerConfig> _servers; // TODO: simplify the structures
    public:
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
};
