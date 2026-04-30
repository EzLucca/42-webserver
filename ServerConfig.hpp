#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <string>
# include <vector>
# include <map>

// This file should store the values from the config file after the configParser.

// This holds the rules that apply ONLY to a specific route (like /upload or /cgi).
// TODO: What variables do you need here based on your .conf file?
struct RouteConfig 
{
    std::string                 path;              // e.g. "/kapouet"
    std::vector<std::string>    allowedMethods;    // GET, POST, etc.
    bool                        autoIndex;         // directory listing on/off
    std::string                 root;              // filesystem root for this route
    std::string                 index;             // default file (e.g. index.html)
    std::string                 redirect;          // if not empty → HTTP redirection
    std::string                 uploadPath;        // where uploads go
};

// Server config will hold the parsed rules from the conf file
// TODO: Add variables for server_name and client_max_body_size
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

#endif
