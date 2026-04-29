#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <string>
# include <vector>
# include <map>


// This holds the rules that apply ONLY to a specific route (like /upload or /cgi).
struct RouteConfig 
{
    // TODO: What variables do you need here based on your .conf file?
  
};

// Server config will hold the parsed rules from the conf file
class ServerConfig 
{
private:
    //Global Server Variables
    int                                 _port;
    std::string                         _host;
    // TODO: Add variables for server_name and client_max_body_size

    //Error Pages Mapping
    // A map is perfect here: Key = HTTP Error Code (e.g., 404), Value = File Path for example "/404.html")
    std::map<int, std::string>          _errorPages;

    // 4. The Routing Table
    // Maps a URL path (like "/upload") to its specific RouteConfig rules.
    std::map<std::string, RouteConfig>  _routes;

public:
    // Constructors & Destructors
    ServerConfig();
    ~ServerConfig();

};

#endif
