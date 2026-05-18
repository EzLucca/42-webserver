#include "ServerManager.hpp"

/**
  * @param server the object to add
  * Add the server object to the server manager pairing it with the name of the server
  */
void    ServerManager::addServer(const ServerConfig & server)
{
    _servers[server.getServerName()].push_back(server);
}

// TEST:
void ServerManager::printServers() const
{
    for (const auto& [key, serverList] : _servers)
    {
        std::cout << "\n=================================\n";
        std::cout << "Map Key: " << key << std::endl;

        for (const auto& server : serverList)
        {
            std::cout << "\n######################\n";

            std::cout << "Port: " << server.getPort() << std::endl;
            std::cout << "Host: " << server.getHost() << std::endl;
            std::cout << "ServerName: " << server.getServerName() << std::endl;
            std::cout << "ClientMaxBodySize: "
                << server.getClientMaxBodySize() << std::endl;

            for (const auto& pair : server.getErrorPages())
            {
                std::cout << "ErrorPage[" << pair.first << "] = "
                    << pair.second << std::endl;
            }

            for (const auto& [routePath, route] : server.getRoutes())
            {
                std::cout << "----------------------" << std::endl;
                std::cout << "  location: " << routePath << std::endl;

                for (const auto& [routeKey, vec] : route.vectorRoute)
                {
                    std::cout << "    " << routeKey << " : ";

                    for (const auto& value : vec)
                        std::cout << value << " ";

                    std::cout << std::endl;
                }

                std::cout << "----------------------" << std::endl;
            }
        }
    }
}

/**
  * @param serverName Name of the server to search
  * @param errorCode specifing the error code
  * Return a string with the path to the errorPage
  */
std::string ServerManager::getServerErrorPages( const std::string& serverName,
        int errorCode) const
{
    const ServerConfig& server = _servers.at(serverName)[0];

    return server.getErrorPage(errorCode);
}

/**
  * @param serverName Name of the server to search
  * @param location key specifing the neste location on the server
  * Return a string of with the pair value of the key excluded errorPages
  */
std::string ServerManager::getServerValues( const std::string& serverName,
        const std::string& key) const
{
    const ServerConfig& server = _servers.at(serverName)[0];
    if (key == "host")
        return server.getHost();

    else if (key == "server_name")
        return server.getServerName();

    else if (key == "listen")
        return std::to_string(server.getPort());

    else if (key == "client_max_body_size")
        return std::to_string(server.getClientMaxBodySize());
    return "";
}

// EXAMPLE:
// std::string root;
// root = server.getServerLocation("mysite.com", "/", "root");
/** 
  * @param serverName Name of the server to search
  * @param location key specifing the neste location on the server
  * @param locationKey the specific key to search
  * Return a string of any location key excluded allowed_methods
  */
std::string ServerManager::getServerLocation( const std::string& serverName,
        const std::string& location,
        const std::string& locationKey) const
{
    const ServerConfig& server = _servers.at(serverName)[0];

    const RouteConfig& route = server.getRoutes().at(location);

    return route.vectorRoute.at(locationKey).at(0);
}

// EXAMPLE:
// std::string post;
// post = server.getServerLocation("mysite.com", "/", "allowed_methods", int value);
// return a string o
/**
  * @param serverName Name of the server to search
  * @param location key specifing the neste location on the server
  * @param locationKey the specific key to search
  * @param method integer correspondig to the method. They are stored in vector.
  * Return a string of any location key excluded allowed_methods
  */
std::string ServerManager::getServerLocationMethods( const std::string& serverName,
        const std::string& location,
        const std::string& locationKey,
        int method) const
{
    const ServerConfig& server = _servers.at(serverName)[0];

    const RouteConfig& route = server.getRoutes().at(location);

    return route.vectorRoute.at(locationKey).at(method);
}
