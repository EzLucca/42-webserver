#include "ServerManager.hpp"

void    ServerManager::addServer(const ServerConfig & server)
{
    _servers.push_back(server);
}

void    ServerManager::printServers() const
{
    for (const auto& server : _servers)  // OK: inside Manager
    {

        std::cout << "\n######################\n" << std::endl;
        std::cout << "Port: " << server.getPort() << std::endl;
        std::cout << "Host: " << server.getHost() << std::endl;
        std::cout << "ServerName: " << server.getServerName() << std::endl;
        std::cout << "ClientMaxBodySize: " << server.getClientMaxBodySize() << std::endl;
        for (const auto& pair : server.getErrorPages())
        {
            std::cout << "ErrorPage[" << pair.first << "] = "
                << pair.second << std::endl;
        }

        for (const auto& [routePath, route] : server.getRoutes())
        {
            std::cout << "----------------------" << std::endl;
            std::cout << "  location: " << routePath << std::endl;

            for (const auto& [key, vec] : route.vectorRoute)
            {
                std::cout << "    " << key << " : ";

                for (const auto& value : vec)
                    std::cout << value << " ";

                std::cout << std::endl;
            }
            std::cout << "----------------------" << std::endl;
        }
    }
}
