#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include <cctype>
#include <fstream> //For ile manipulation
#include <sstream> //For ile manipulation
#include <string>
#include <vector>

//  static void printconfig(ServerConfig config) // DEBUG:
//  {
//      // to print
//      std::cout << config.getPort() << std::endl;
//      std::cout << config.getHost() << std::endl;
//      std::cout << config.getServerName() << std::endl;
//      std::cout << config.getClientMaxBodySize() << std::endl;
//      std::cout << config.getErrorPages() << std::endl;
//      std::cout << std::endl;
// 
//      const auto& routes = config.getRoutes();
// 
//      for (const auto& [path, route] : routes) {
//          std::cout << "Route path: " << path << "\n";
//          std::cout << "Root: " << route.root << "\n";
//          std::cout << "Index: " << route.index << "\n";
//          std::cout << "Methods: ";
//          for (const auto& m : route.allowedMethods)
//              std::cout << m << " ";
//          std::cout << "\nAutoIndex: " << route.autoIndex << "\n\n";
//      }
//  }

ConfigParser::ConfigParser() {
    std::cout << "ConfigParser constructor called.\n" << std::endl;
}

ConfigParser::~ConfigParser() {
    std::cout << "ConfigParser destructor called." << std::endl;
}

static int openFile(const std::string &file_path, std::fstream *fstream) {
    if (fstream->is_open())
        fstream->close();

    fstream->open(file_path.c_str(), std::ios::in);
    if (!fstream->is_open()) {
        fstream->clear();
        return (1);
    }
    return (0);
}

void ConfigParser::setConfigBuffer(std::string line) {
    _configBuffer.append(line);
    _configBuffer.append("\n");
}

void ConfigParser::setConfigLocations(ServerConfig& config, std::string workingBuffer, size_t& pos)
{
    while (true) {
        size_t found = workingBuffer.find("location", pos);
        if (found == std::string::npos)
            break;

        RouteConfig route; // create a fresh RouteConfig each iteration
        route.path = findConfigKey<std::string>("location", workingBuffer, pos);
        // check for {
        // Optional keys: root, index, allow_methods, autoindex
        try {
            route.root = findConfigKey<std::string>("root", workingBuffer, pos);
            // TODO: validate root folder
        } catch (...) { route.root = ""; }

        try {
            route.index = findConfigKey<std::string>("index", workingBuffer, pos);
        } catch (...) { route.index = ""; }

        try {
            std::string methodsStr = findConfigKey<std::string>("allow_methods", workingBuffer, pos);
            std::istringstream iss(methodsStr);
            std::string method;
            while (iss >> method) {
                // TODO: validate methods GET POST DELETE check if location allows it
                route.allowedMethods.push_back(method);
            }
        } catch (...) { route.allowedMethods.clear(); }

        try {
            std::string autoIndexStr = findConfigKey<std::string>("autoindex", workingBuffer, pos);
            route.autoIndex = (autoIndexStr == "on");
        } catch (...) { route.autoIndex = false; }

        // Save this route into the ServerConfig _routes map
        config.setRoute(route.path, route);
    }
    }

    void ConfigParser::setConfigContext(ServerConfig& config, std::string workingBuffer, size_t& pos) {

        int port = findConfigKey<int>("listen", workingBuffer, pos);
        // TODO: range of ports 1024 - 49151

        std::string host = findConfigKey<std::string>("host", workingBuffer, pos);
        std::string  serverName = findConfigKey<std::string>("server_name", workingBuffer, pos);        // awesomeserver

        size_t  clientMaxBodySize = findConfigKey<size_t>("client_max_body_size", workingBuffer, pos);
        // TODO: check for unit type.
        // 10M or 10MB

        config.setPort(port);
        config.setHost(host);
        config.setServerName(serverName);
        config.setClientMaxBodySize(clientMaxBodySize);

        // getting all the error pages
        while (true) {
            size_t found = workingBuffer.find("error_page", pos);
            if (found == std::string::npos)
                break;
            std::string errorPages = findConfigKey<std::string>("error_page", workingBuffer, pos);
            std::istringstream iss(errorPages);
            int errorCode;
            std::string errorPath;

            iss >> errorCode >> errorPath;
            // TODO: check errorpath 
            config.setErrorPage(errorCode, errorPath);
        }
        // setConfigLocations(config, workingBuffer, config.pos);
        setConfigLocations(config, workingBuffer, pos);

        // printconfig(config); // DEBUG:
    }

    std::string ConfigParser::getConfigBuffer() { return (_configBuffer); }

    // IMPORTANT: Parse should receive the manager object
    // int ConfigParser::parse(std::string configFile, ServerManager& server) 
    int ConfigParser::parse(std::string configFile, ServerConfig& config) 
    {

        std::fstream fstream;
        if (configFile.empty() || openFile(configFile, &fstream)) {
            std::cerr << "Config file name is not correct." << std::endl;
            return (0);
        }
        std::string line;

        std::ifstream file(configFile);
        while (std::getline(file, line)) // Write everything to our string object
        {
            size_t pos = line.find("#");
            if (pos != std::string::npos)
            {
                line = line.substr(0, pos);
            }
            setConfigBuffer(line);
            // buffer with everything by append line
        }
        // workingBuffer now have all the configfile
        const std::string &workingBuffer = getConfigBuffer();

        // creating the ServerConfig object
        // get blocks
        size_t pos = 0;
        while (true) {
            // ServerConfig config;
            // config.pos = 0; // track position
            size_t found = workingBuffer.find("server {", pos); // server { for validation
            if (found == std::string::npos)
                break;
            setConfigContext(config, workingBuffer, pos); 
            // server.addServer(config);
            // std::cout << pos << std::endl;
        }
        // printconfig(config); // DEBUG:
        return 0;
        }
