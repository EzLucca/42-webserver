#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
{
}
ServerConfig::~ServerConfig()
{
}
// setters
void    ServerConfig::setPort(int port)
{
    _port = port;
}
void    ServerConfig::setHost(std::string host)
{
    _host = host;
}
void    ServerConfig::setServerName(std::string serverName)
{
    _serverName = serverName;
}
void    ServerConfig::setClientMaxBodySize(size_t clientMaxBodySize)
{
    _clientMaxBodySize = clientMaxBodySize;
}
void    ServerConfig::setErrorPage(int code, const std::string& errorPage)
{
    _errorPages[code] = errorPage;
}
void    ServerConfig::setRoute(const std::string& location, RouteConfig  routes)
{
    routes.path = location;
}

// getters
int                                         ServerConfig::getPort() const
{
    return (_port);
}
const std::string                           ServerConfig::getHost() const
{
    return (_host);
}
const std::string                           ServerConfig::getServerName() const
{
    return (_serverName);
}
size_t                                      ServerConfig::getClientMaxBodySize() const
{
    return (_clientMaxBodySize);
}
const std::map<int, std::string>&           ServerConfig::getErrorPages() const
{
    return (_errorPages);
}
const std::map<std::string, RouteConfig>&   ServerConfig::getRoutes() const
{
    return (_routes);
}

// Useful helpers
// std::string         ServerConfig::getErrorPage(int code) const
// {
// }
// const RouteConfig*  ServerConfig::getRoute(const std::string& location) const
// {
// }
