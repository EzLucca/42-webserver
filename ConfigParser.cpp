#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include <cctype>
#include <fstream> //For ile manipulation
#include <sstream> //For ile manipulation
#include <string>
#include <vector>

// static void printconfig(ServerConfig config) // TEST:
// {
//     // to print
//     std::cout << config.getPort() << std::endl;
//     std::cout << config.getHost() << std::endl;
//     std::cout << config.getServerName() << std::endl;
//     std::cout << config.getClientMaxBodySize() << std::endl;
//     std::cout << config.getErrorPages() << std::endl;
//     std::cout << std::endl;
//
//     const auto& routes = config.getRoutes();
//
//     for (const auto& [path, route] : routes) {
//         std::cout << "Route path: " << path << "\n";
//         std::cout << "Root: " << route.root << "\n";
//         std::cout << "Index: " << route.index << "\n";
//         std::cout << "Methods: ";
//         for (const auto& m : route.allowedMethods)
//             std::cout << m << " ";
//         std::cout << "\nAutoIndex: " << route.autoIndex << "\n\n";
//     }
// }

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

void ConfigParser::setConfigLocations(ServerConfig& config, std::string workingBuffer, size_t& pos)
{
    while (true) {
        size_t found = workingBuffer.find("location", pos);
        if (found == std::string::npos)
            break;

        RouteConfig route; // create a fresh RouteConfig each iteration
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
    }
}

// std::map<std::string, std::string> getContextKeys(const std::string& buffer)
// {
//     std::map<std::string, std::string> directivesMap;
//     std::istringstream stream(buffer);
//     std::string line;
//
//     while (std::getline(stream, line)) {
//         // Skip empty lines
//         if (line.empty()) continue;
//
//         // std::cout << line << std::endl;
//         std::istringstream lineStream(line);
//         std::string token;
//         while (lineStream >> token)
//         {
//             // pair the tokens
//             if (token == "#") break;
//             std::cout << token << std::endl;
//         }
//         // if (key.empty()) continue;
//         // Insert into the map
//         // directivesMap[key] = value;
//     }
//     // for (const auto& [key, value] : directivesMap) {
//     //     std::cout << key << " : " << value << std::endl;
//     // }
//     return directivesMap;
// }

std::map<std::string, std::string> getContextKeys(const std::string& buffer)
{
    std::map<std::string, std::string> directivesMap;
    std::istringstream stream(buffer);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        std::istringstream lineStream(line);
        std::string key, value, token;

        // Read key
        if (!(lineStream >> key)) continue;

        // Stop if line starts with comment
        if (key == "#") continue;

        // Read value
        if (!(lineStream >> value)) continue;
        if (value.find(";"))
        {
            std::cout << value << std::endl;

        }
        // store more values to each key. use vector?
        // Ignore rest of line after '#'
        while (lineStream >> token) {
            if (token == "#") break;
        }

        directivesMap[key] = value;
        std::cout << key << " : " << value << std::endl;
    }

    return directivesMap;
}

void ConfigParser::setConfigContext(ServerConfig& config, std::string workingBuffer, size_t& pos) {

    std::map<std::string, std::string>  keys = getContextKeys(workingBuffer);
    // std::cout << keys << std::endl;
    int port = findConfigKey<int>("listen", workingBuffer, pos);
    std::string host = findConfigKey<std::string>("host", workingBuffer, pos);
    std::string  serverName = findConfigKey<std::string>("server_name", workingBuffer, pos);        // awesomeserver
    size_t  clientMaxBodySize = findConfigKey<size_t>("client_max_body_size", workingBuffer, pos);

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
        config.setErrorPage(errorCode, errorPath);
    }
    setConfigLocations(config, workingBuffer, config.pos);
}

std::string ConfigParser::getConfigBuffer() { return (_configBuffer); }

int ConfigParser::parse(std::string configFile) {

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

    std::map<std::string, std::string>  keys = getContextKeys(workingBuffer);
    // std::cout << keys << std::endl;
    exit(1);

    // creating the ServerConfig object
    ServerConfig config;

    // get blocks
    config.pos = 0; // track position
    while (true) {
        size_t found = workingBuffer.find("server {", config.pos); // server { for validation
        if (found == std::string::npos)
            break;
        setConfigContext(config, workingBuffer, config.pos); 
    }
    // printconfig(config); // TEST:
    return 0;
    }
