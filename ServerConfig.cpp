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
}

// getters
int                                         getPort() const;
const std::string                           getHost() const;
const std::string                           getServerName() const;        // awesomeserver
size_t                                      getClientMaxBodySize() const;
const std::map<int, std::string>&           getErrorPages() const;
const std::map<std::string, RouteConfig>&   getRoutes() const;


// Useful helpers
std::string         getErrorPage(int code) const;
const RouteConfig*  getRoute(const std::string& location) const;
