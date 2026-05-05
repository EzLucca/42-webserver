#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include <cctype>
#include <fstream> //For ile manipulation
#include <sstream> //For ile manipulation
#include <string>
#include <vector>

ConfigParser::ConfigParser() {
    std::cout << "ConfigParser constructor called." << std::endl;
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

std::string ConfigParser::getConfigBuffer() { return (_configBuffer); }

int ConfigParser::parse(std::string configFile) {
    // logic of parsing
    // read the config file

    std::fstream fstream;
    if (configFile.empty() || openFile(configFile, &fstream)) {
        std::cerr << "Config file name is not correct." << std::endl;
        return (0);
    }
    std::string line;

    std::ifstream file(configFile);
    while (std::getline(file, line)) // Write everything to our string object
    {
        // buffer with everything by append line
        setConfigBuffer(line);
    }
    // workingBuffer now have all the configfile
    const std::string &workingBuffer = getConfigBuffer();

    // creating the ServerConfig object
    ServerConfig config;

    size_t pos = 0; // track position
    int port = findConfigKey<int>("listen", workingBuffer, pos);
    std::string host = findConfigKey<std::string>("host", workingBuffer, pos);
    std::string  serverName = findConfigKey<std::string>("server_name", workingBuffer, pos);        // awesomeserver
    size_t  clientMaxBodySize = findConfigKey<size_t>("client_max_body_size", workingBuffer, pos);

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
        config.setErrorPage(errorCode, errorPath);
    }

    config.setPort(port);
    config.setHost(host);
    config.setServerName(serverName);
    config.setClientMaxBodySize(clientMaxBodySize);

    std::cout << config.getPort() << std::endl;
    std::cout << config.getHost() << std::endl;
    std::cout << config.getServerName() << std::endl;
    std::cout << config.getClientMaxBodySize() << std::endl;
    std::cout << config.getErrorPages() << std::endl;

    // containers 
    while (true) {
        size_t found = workingBuffer.find("location", pos);
        if (found == std::string::npos)
            break;

        RouteConfig route; // create a fresh RouteConfig each iteration

        // Parse location path
        route.path = findConfigKey<std::string>("location", workingBuffer, pos);

        // Optional keys: root, index, allow_methods, autoindex
        try {
            route.root = findConfigKey<std::string>("root", workingBuffer, pos);
        } catch (...) { route.root = ""; }

        try {
            route.index = findConfigKey<std::string>("index", workingBuffer, pos);
        } catch (...) { route.index = ""; }

        try {
            std::string methodsStr = findConfigKey<std::string>("allow_methods", workingBuffer, pos);
            std::istringstream iss(methodsStr);
            std::string method;
            while (iss >> method) {
                route.allowedMethods.push_back(method);
            }
        } catch (...) { route.allowedMethods.clear(); }

        try {
            std::string autoIndexStr = findConfigKey<std::string>("autoindex", workingBuffer, pos);
            route.autoIndex = (autoIndexStr == "on");
        } catch (...) { route.autoIndex = false; }

        // Save this route into the ServerConfig _routes map
        config.setRoute(route.path, route);
        const auto& routes = config.getRoutes();

        for (const auto& [path, route] : routes) {
            std::cout << "Route path: " << path << "\n";
            std::cout << "Root: " << route.root << "\n";
            std::cout << "Index: " << route.index << "\n";
            std::cout << "Methods: ";
            for (const auto& m : route.allowedMethods)
                std::cout << m << " ";
            std::cout << "\nAutoIndex: " << route.autoIndex << "\n\n";
        }
    }

    // cgi
    return 0;
}
