#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "HttpParser.hpp"
#include <cctype>
#include <fstream> //For ile manipulation
#include <sstream> //For ile manipulation
#include <string>
#include <vector>
#include <filesystem>

static void printServerConfig(ServerConfig config)
{
    // TEST: remove after
    std::cout << "Port: " << config.getPort() << std::endl;
    std::cout << "Host: " << config.getHost() << std::endl;
    std::cout << "ServerName: " << config.getServerName() << std::endl;
    std::cout << "ClientMaxBodySize: " << config.getClientMaxBodySize() << std::endl;
    for (const auto& pair : config.getErrorPages())
    {
        std::cout << "ErrorPage[" << pair.first << "] = "
            << pair.second << std::endl;
    }

    for (const auto& [routePath, route] : config.getRoutes())
    {
        std::cout << "----------------------" << std::endl;
        std::cout << "  location: " << routePath << std::endl;

        for (const auto& [key, vec] : route.vectorRoute)
        {
            std::cout << "    " << key << " : ";

            for (const auto& value : vec)
                std::cout << value << " ";

            std::cout << std::endl;
        }
        std::cout << "----------------------" << std::endl;
    }
}

void    setDirectives(ServerConfig &config, std::map<std::string, std::string> &values)
{
    config.setPort(stoi(values["listen"]));
    config.setHost(values["host"]);
    config.setServerName(values["server_name"]);

    // check for body size in MB
    if (values["client_max_body_size"].find("M") != std::string::npos)
    {
        config.setClientMaxBodySize(stoi(values["client_max_body_size"]));
    }
    else
        config.setClientMaxBodySize(0);

}

std::string ConfigParser::trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");

    if (start == std::string::npos)
        return "";

    return str.substr(start, end - start + 1);
}

void ConfigParser::parselocation(std::istringstream &stream, size_t& pos, RouteConfig &config)
{
    std::string key;
    std::string value;

    while (stream >> key)
    {
        if (key == "}")
            break;

        pos += key.size();

        std::getline(stream, value);

        value = trim(value);

        // remove ';'
        if (!value.empty() && value.back() == ';')
            value.pop_back();

        if (key == "allowed_methods")
        {
            std::stringstream ss(value);
            std::string word;

            while (ss >> word)
                config.vectorRoute[key].push_back(word);
            continue;
        }
        config.vectorRoute[key].push_back(value);
    }
}

void    ConfigParser::secondparse(ServerConfig& config, std::string workingBuffer, size_t& pos)
{
    std::map<std::string, std::string> values;
    std::istringstream stream(workingBuffer);
    std::string key;
    std::string value;
    std::string line;

    while (std::getline(stream, line)) {
        std::istringstream linestream(line);
        std::string key, value;

        linestream >> key;
        pos += key.size();
        if(key == "server" || key.empty())
            continue;
        std::getline(linestream, value);

        value = trim(value);

        // remove trailing ';'
        if (!value.empty() && value.back() == ';')
            value.pop_back();

        values[key] = value;

        if (key == "error_page")
        {
            std::istringstream iss(value);

            int errorCode;
            std::string errorPath;

            iss >> errorCode >> errorPath;

            config.setErrorPage(errorCode, errorPath);

            continue;
        }
        while (key == "location")
        {
            RouteConfig nestedLocation;

            if (value.find("{") == std::string::npos)
            {
                std::cout << "{ location not found" << std::endl;
                exit(2);
            }

            size_t locationPos = value.find(" ");
            std::string locationValue = value.substr(0, locationPos);

            parselocation(stream, pos, nestedLocation);
            config.setRoute(locationValue, nestedLocation);
            break;
        }

    }

    setDirectives(config, values);
    // function to set the values from the map to the config object
    printServerConfig(config);
}

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

// static int checkExistance(std::string filePath)
// {
//     if(!std::filesystem::exists(filePath))
//     {
//         std::cout << filePath << std::endl; 
//         std::cout << "The path is invalid!" << std::endl;
//         exit(2);
//     }
//     return 0;
// }

void ConfigParser::setConfigBuffer(std::string line) {
    _configBuffer.append(line);
    _configBuffer.append("\n");
}

// void ConfigParser::setConfigLocations(ServerConfig& config, std::string workingBuffer, size_t& pos)
// {
//     while (true) {
//         size_t found = workingBuffer.find("location", pos);
//         if (found == std::string::npos)
//             break;
//
//         RouteConfig route; // create a fresh RouteConfig each iteration
//         route.path = findConfigKey<std::string>("location", workingBuffer, pos);
//         // check for {
//         // Optional keys: root, index, allow_methods, autoindex
//         try {
//             route.root = findConfigKey<std::string>("root", workingBuffer, pos);
//             // TODO: validate root folder
//             checkExistance(route.root);
//         } catch (...) { route.root = ""; }
//
//         try {
//             route.index = findConfigKey<std::string>("index", workingBuffer, pos);
//         } catch (...) { route.index = ""; }
//
//         try {
//             std::string methodsStr = findConfigKey<std::string>("allow_methods", workingBuffer, pos);
//             std::istringstream iss(methodsStr);
//             std::string method;
//             while (iss >> method) {
//                 // TODO: validate methods GET POST DELETE check if location allows it
//
//                 route.allowedMethods.push_back(method);
//             }
//         } catch (...) { route.allowedMethods.clear(); }
//
//         try {
//             std::string autoIndexStr = findConfigKey<std::string>("autoindex", workingBuffer, pos);
//             route.autoIndex = (autoIndexStr == "on");
//         } catch (...) { route.autoIndex = false; }
//
//         // Save this route into the ServerConfig _routes map
//         config.setRoute(route.path, route);
//     }
// }

// void ConfigParser::setConfigContext(ServerConfig& config, std::string workingBuffer, size_t& pos)
// {
//     int port = findConfigKey<int>("listen", workingBuffer, pos);
//     // TODO: range of ports 1024 - 49151
//     if (port < 1024 || port > 49151)
//     {
//         std::cerr << "port out of range." << std::endl;
//         exit(2);
//     }
//
//     std::string host = findConfigKey<std::string>("host", workingBuffer, pos);
//     std::string  serverName = findConfigKey<std::string>("server_name", workingBuffer, pos);        // awesomeserver
//
//     size_t  clientMaxBodySize = findConfigKey<size_t>("client_max_body_size", workingBuffer, pos);
//     // TODO: check for unit type.
//     // 10M or 10MB
//
//     config.setPort(port);
//     config.setHost(host);
//     config.setServerName(serverName);
//     config.setClientMaxBodySize(clientMaxBodySize);
//
//     // getting all the error pages
//     while (true) {
//         size_t found = workingBuffer.find("error_page", pos);
//         if (found == std::string::npos)
//             break;
//         std::string errorPages = findConfigKey<std::string>("error_page", workingBuffer, pos);
//         std::istringstream iss(errorPages);
//         int errorCode;
//         std::string errorPath;
//
//         iss >> errorCode >> errorPath;
//         // TODO: check errorpath
//         checkExistance(errorPath);
//
//         config.setErrorPage(errorCode, errorPath);
//     }
//     // setConfigLocations(config, workingBuffer, config.pos);
//     setConfigLocations(config, workingBuffer, pos);
//
//     // printconfig(config); // DEBUG:
// }

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

    size_t pos = 0;

    // TEST:
    try{
        secondparse(config, workingBuffer, pos);
    }
    catch (const std::exception& e) 
    {
        std::cout << e.what() << " (testing)";
    }
    exit(1);
    // ~TEST:


    // creating the ServerConfig object
    // get blocks
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
