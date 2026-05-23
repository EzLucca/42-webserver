#include "ServerManager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <arpa/inet.h>

/**
 * @param server the object to add
 * Add the server object to the server manager pairing it with the name of the server
 */
void    ServerManager::addServer(const ServerConfig & server)
{
    _servers.push_back(server);
    _servercount++;
    // _servers.push_back(server);
}

std::vector<ServerConfig> ServerManager::getServers()
{
    return (_servers);
}

// TEST:
void ServerManager::printServers() const
{
    
    for (size_t i = 0; i < _servers.size(); ++i)
    {
        const ServerConfig& server = _servers[i];

        std::cout << "\n=================================\n";
        std::cout << "Server #" << (i + 1) << "\n";
        std::cout << "=================================\n";

        std::cout << "Port: " << server.getPort() << std::endl;
        std::cout << "Host: " << server.getHost() << std::endl;
        std::cout << "ServerName: " << server.getServerName() << std::endl;
        std::cout << "ClientMaxBodySize: " << server.getClientMaxBodySize() << std::endl;

        // C++98 tapa iteroida map-rakennetta (Error Pages)
        std::map<int, std::string>::const_iterator errIt;
        const std::map<int, std::string>& errorPages = server.getErrorPages();
        
        for (errIt = errorPages.begin(); errIt != errorPages.end(); ++errIt)
        {
            std::cout << "ErrorPage[" << errIt->first << "] = "
                      << errIt->second << std::endl;
        }

       
        std::map<std::string, RouteConfig>::const_iterator routeIt;
        const std::map<std::string, RouteConfig>& routes = server.getRoutes();
        
        for (routeIt = routes.begin(); routeIt != routes.end(); ++routeIt)
        {
            std::cout << "----------------------" << std::endl;
            std::cout << "  location: " << routeIt->first << std::endl;

            const RouteConfig& route = routeIt->second;
             std::unordered_map<std::string, std::vector<std::string>>::const_iterator vecIt;

            for (vecIt = route.vectorRoute.begin(); vecIt != route.vectorRoute.end(); ++vecIt)
            {
                std::cout << "    " << vecIt->first << " : ";

                const std::vector<std::string>& vec = vecIt->second;
                for (size_t j = 0; j < vec.size(); ++j)
                {
                    std::cout << vec[j] << " ";
                }
                std::cout << std::endl;
            }
            std::cout << "----------------------" << std::endl;
        }
    }
}

int    ServerManager::getServerCount() const
{
    return (_servercount);
}

const ServerConfig* ServerManager::getServerByFd(int fd)
{
    std::map<int, const ServerConfig*>::iterator it = _masterSocketRegistry.find(fd);
            if (it != _masterSocketRegistry.end()) 
            {
                const ServerConfig* matchedConfig = it->second;
                return (matchedConfig);
            }
    return (NULL); // check for the null if it cant find the server
}

