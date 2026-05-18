#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <iostream>
#include "ServerConfig.hpp"
#include "ServerManager.hpp"

// Reads the text file, runs tokenization state machine, and spits out seerverconfig objects
class ConfigParser
{
    private:
        std::string _configBuffer;
        RouteConfig _routes;

    public:
        ConfigParser();
        ~ConfigParser();
        bool        isBlockEnd(const std::string& line);
        bool        isServerStart(const std::string& line);
        bool        parseConfig(ServerConfig& config, std::istream& stream);
        bool        shouldSkipLine(const std::string& line);
        int         parse(std::string configFile, ServerManager& server);
        std::string getConfigBuffer();
        std::string trim(const std::string& str);
        void        addRoute(const RouteConfig& route);
        void        parseDirective( const std::string& line, std::istream& stream, ServerConfig& config, std::map<std::string, std::string>& values);
        void        parseErrorPage( const std::string& value, ServerConfig& config);
        void        parseLocationBlock( const std::string& value, std::istream& stream, ServerConfig& config);
        void        parselocation(std::istream &stream, RouteConfig &nestedLocation, ServerConfig &config);
        void        setConfigBuffer(std::string line);
};

#endif
