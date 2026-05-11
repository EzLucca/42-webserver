#ifndef SERVERMANAGER_HPP
# define SERVERMANAGER_HPP

# include <iostream>
# include "ServerConfig.hpp"


// grand conductor, our waiter! all the poll logic will be here 
class ServerManager
{
    private:
        std::vector<ServerConfig> _servers;
    public:
        void    addServer(const ServerConfig & server);
        void    printServers() const;
};

#endif
