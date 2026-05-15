#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <string>
# include <vector>
# include <map>
# include <ostream>
# include <iostream>
# include <stdbool.h>
# include <unordered_map>

struct RouteConfig 
{
    // TODO: check if unordered is necessary
    std::unordered_map<std::string, std::vector<std::string>> vectorRoute;
};

// Server config will hold the parsed rules from the conf file
class ServerConfig 
{
    private:
        //Global Server Variables
        int                                 _port;              // 8080
        std::string                         _host;              // 127.0.0.1
        std::string                         _serverName;        // awesomeserver
        size_t                              _clientMaxBodySize;
        std::map<int, std::string>          _errorPages;        // indexed error pages
        std::map<std::string, RouteConfig>  _routes;            // routing tables

    public:
        // Constructors & Destructors
        ServerConfig();
        ~ServerConfig();

        size_t pos;

        // setters
        void    setPort(int port);
        void    setHost(std::string host);
        void    setServerName(std::string serverName);                  // awesomeserver
        void    setClientMaxBodySize(size_t clientMaxBodySize);
        void    setErrorPage(int code, const std::string& errorPage);        // indexed error pages
        void    setRoute(const std::string& location, RouteConfig  routes);  // routing tables

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
};

std::ostream& operator<<(std::ostream& os, const std::map<int, std::string>& m);
#endif
