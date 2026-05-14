#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include "HttpParser.hpp"
#include <cctype>
#include <fstream> //For ile manipulation
#include <sstream> //For ile manipulation
#include <string>
#include <vector>
#include <filesystem>
#include <arpa/inet.h>

static int checkExistance(std::string filePath)
{
    if(!std::filesystem::exists(filePath))
    {
        std::cout << filePath << std::endl; 
        std::cout << "The path is invalid!" << std::endl;
        exit(2);
    }
    return 0;
}

void    setDirectives(ServerConfig &config, std::map<std::string, std::string> &values)
{
    // TODO: port validation
    if(values["listen"].empty())
        std::cerr << "port empty." << std::endl;
    int port = stoi(values["listen"]);
    if (port < 1024 || port > 49151)
    {
        std::cerr << "port out of range." << std::endl;
        exit(2);
    }
    config.setPort(port);

    // TODO: host validation
    std::string host = values["host"];
    struct sockaddr_in sa;

    if(inet_pton(AF_INET, host.c_str(), &(sa.sin_addr)) == 1)
        config.setHost(host);
    else
        std::cerr << "host invalid" << std::endl;

    // TODO: server name validation
    config.setServerName(values["server_name"]);

    // TODO: client max body size validation
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

void ConfigParser::parselocation(std::istream &stream, RouteConfig &config)
{
    std::string key;
    std::string value;

    while (stream >> key)
    {
        if (key == "}")
            break;

        std::getline(stream, value);

        value = trim(value);

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

bool ConfigParser::shouldSkipLine(const std::string& line)
{
    return line.empty();
}

bool ConfigParser::isBlockEnd(const std::string& line)
{
    return line == "}";
}

bool ConfigParser::isServerStart(const std::string& line)
{
    return line == "server {";
}

void ConfigParser::parseErrorPage( const std::string& value, ServerConfig& config)
{
    std::istringstream iss(value);

    int errorCode;
    std::string errorPath;

    iss >> errorCode >> errorPath;

    checkExistance(errorPath);
    config.setErrorPage(errorCode, errorPath);
}

// TODO: validate location, root, cig_pass and methods.
void ConfigParser::parseLocationBlock( const std::string& value, std::istream& stream, ServerConfig& config)
{
    RouteConfig nestedLocation;

    if (value.find("{") == std::string::npos)
        throw std::runtime_error("{ location not found");

    size_t locationPos = value.find(" ");

    std::string locationValue = value.substr(0, locationPos);
    // checkExistance(locationValue);

    parselocation(stream, nestedLocation);

    config.setRoute(locationValue, nestedLocation);
}

void ConfigParser::parseDirective( const std::string& line, std::istream& stream, ServerConfig& config, std::map<std::string, std::string>& values)
{
    std::istringstream linestream(line);

    std::string key;
    std::string value;

    linestream >> key;

    std::getline(linestream, value);

    value = trim(value);

    if (!value.empty() && value.back() == ';')
        value.pop_back();
    if (key == "error_page")
    {
        parseErrorPage(value, config);
        return;
    }
    if (key == "location")
    {
        parseLocationBlock(value, stream, config);
        return;
    }
    values[key] = value;
}

bool ConfigParser::parseConfig(ServerConfig& config, std::istream& stream)
{
    std::map<std::string, std::string> values;

    std::string line;
    bool foundServer = false;

    while (std::getline(stream, line))
    {
        line = trim(line);

        if (shouldSkipLine(line))
            continue;
        if (isBlockEnd(line))
            break;
        if (isServerStart(line))
        {
            foundServer = true;
            continue;
        }
        parseDirective(line, stream, config, values);
    }
    if (!foundServer)
        return false;
    setDirectives(config, values);
    return true;
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

void ConfigParser::setConfigBuffer(std::string line) {
    _configBuffer.append(line);
    _configBuffer.append("\n");
}

std::string ConfigParser::getConfigBuffer() { return (_configBuffer); }

// IMPORTANT: Parse should receive the manager object
int ConfigParser::parse(std::string configFile, ServerManager& server) 
{
    // TODO: check if file is open and permissions
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

    // TEST:
    try{
        std::istringstream stream(workingBuffer);

        while (stream)
        {
            ServerConfig config;

            if(!parseConfig(config, stream))
                break;
            server.addServer(config);
        }
        server.printServers();
    }
    catch (const std::exception& e) 
    {
        std::cout << e.what() << " (testing)";
    }
    exit(1);
    // ~TEST:

    return 0;
}
