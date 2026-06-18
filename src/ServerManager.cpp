#include "ServerManager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <arpa/inet.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

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

std::vector<int> ServerManager::getMasterFds() const
{
    std::vector<int> masterFds;

    for (std::map<int, const ServerConfig*>::const_iterator it = _masterSocketRegistry.begin(); 
            it != _masterSocketRegistry.end(); 
            ++it)
    {
        masterFds.push_back(it->first);
    }
    return masterFds;
}

bool ServerManager::setupMasterSockets(struct pollfd fds[], std::map<int, const ServerConfig*>& masterSocketRegistry)
{
    for (size_t i = 0; i < _servers.size(); ++i)
    {
        std::cout << "Setting up Master Socket for port: "
            << _servers[i].getPort() << std::endl;

        // create master socket
        // AF_INET = IPv4, SOCK_STREAM = TCP
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
        {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        int opt = 1; // works as 1/0 switch
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
                0) // set socket options
            std::cerr << "setsockopt failed."
                << std::endl;

        if (fcntl(server_fd, F_SETFL, O_NONBLOCK) <
                0) // set file status flags to nonblocking
        {
            std::cerr << "fcntl failed." << std::endl;
            close(server_fd);
            return false;
        }

        // define address and port (bind)
        struct sockaddr_in address;
        std::memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY; // Listen all interfaces CHECK THIS
        address.sin_port = htons(
                _servers[i]
                .getPort()); // hardcoded
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            std::cerr << "Bind failed. Is the port already in use?" << std::endl;
            close(server_fd);
            return false;
        }

        // with listen we transform default active socket into passice socket
        // (server mode)
        //  also initializes queue for in case of client rush. Somaxconn macro gives
        //  us largest queue
        if (listen(server_fd, SOMAXCONN) < 0)
        {
            std::cerr << "Listen failed" << std::endl;
            close(server_fd);
            return false;
        }

        // set master socket in the first index
        fds[i].fd = server_fd;
        fds[i].events = POLLIN; // POLLIN means tell me when there is data to read
        masterSocketRegistry[server_fd] = &_servers[i];
        _masterSocketRegistry[server_fd] = &_servers[i];
    }

    return true;
}

const std::map<int, const ServerConfig*>& ServerManager::getMasterSocketRegistry() const
{
    return _masterSocketRegistry;
}

void ServerManager::shutDownServers()
{
    for (std::map<int, const ServerConfig*>::const_iterator it =
            _masterSocketRegistry.begin();
            it != _masterSocketRegistry.end();
            ++it)
    {
        int fd = it->first;

        if (fd >= 0)
        {
            close(fd);
            std::cout << "Closed master socket " << fd << '\n';
        }
    }
}

