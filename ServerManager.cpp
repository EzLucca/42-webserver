#include "ServerManager.hpp"

void    ServerManager::addServer(const ServerConfig & server)
{
    _servers.push_back(server);
}

void    ServerManager::printServers() const
{
    for (const auto& server : _servers)  // OK: inside Manager
    {
        std::cout << server.getServerName() << " : " << server.getPort() << std::endl;
    }
}
