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
void    ServerConfig::setRoute(const std::string& location, RouteConfig route) {
    _routes[location] = route;
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

std::ostream& operator<<(std::ostream& os, const std::map<int, std::string>& m) {
    os << "{";
    bool first = true;
    for (const auto& [key, value] : m) {
        if (!first) os << ", ";
        os << key << ": " << value;
        first = false;
    }
    os << "}";
    return os;
}

// Useful helpers
std::string         ServerConfig::getErrorPage(int code) const
{
    std::map<int, std::string>::const_iterator it = _errorPages.find(code);

    if (it != _errorPages.end())
    {
        return it->second;
    }
    return "";
}

const RouteConfig*  ServerConfig::getRoute(const std::string& key) const
{
    std::map<std::string, RouteConfig>::const_iterator it = _routes.find(key);

    if (it != _routes.end())
    {
        return &it->second;
    }
    return NULL;
}

void    ServerConfig::setLocationlist(std::string location)
{
    _locationList.push_back(location);
}

std::vector<std::string>    ServerConfig::getLocationList() const
{
    return (_locationList);
}
