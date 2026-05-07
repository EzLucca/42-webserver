#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include <cctype>
#include <fstream> //For ile manipulation
#include <sstream> //For ile manipulation
#include <string>
#include <vector>

static void printconfig(ServerConfig config) // DEBUG:
{
    // to print
    std::cout << config.getPort() << std::endl;
    std::cout << config.getHost() << std::endl;
    std::cout << config.getServerName() << std::endl;
    std::cout << config.getClientMaxBodySize() << std::endl;
    std::cout << config.getErrorPages() << std::endl;
    std::cout << std::endl;

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
    template <typename T>
static void printvar(const T& var)
{
    std::cout << var << std::endl;
}

std::vector<std::string> tokenize(const std::string& input)
{
    std::vector<std::string> tokens;
    std::string current;

    for (size_t i = 0; i < input.size(); ++i)
    {
        char c = input[i];

        // Remove comments
        if (c == '#')
        {
            while (i < input.size() && input[i] != '\n')
                ++i;

            continue;
        }

        // Whitespace
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        // Special symbols
        else if (c == '{' || c == '}' || c == ';')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }

            tokens.push_back(std::string(1, c));
        }
        else
        {
            current += c;
        }
    }

    // Last token
    if (!current.empty())
        tokens.push_back(current);

    // Print tokens DEBUG:
    // for (const std::string& token : tokens)
    //     std::cout << token << '\n';

    return tokens;
}

Block parseConfig(const std::string& config)
{
    std::vector<std::string> tokens = tokenize(config);

    // To this point I have the tokens as a vector of strings
    Block root;
    root.name = "root";

    std::stack<Block*> blockStack;
    blockStack.push(&root);

    size_t i = 0;

    while (i < tokens.size())
    {
        if (tokens[i] == "}")
        {
            blockStack.pop();
            ++i;
            continue;
        }

        std::string name = tokens[i++];
        std::vector<std::string> args;

        // stores in the args vector of strings the values of each key.
        while (i < tokens.size() && tokens[i] != "{" && tokens[i] != ";")
        {
            args.push_back(tokens[i++]);
        }

        // BLOCK
        if (i < tokens.size() && tokens[i] == "{")
        {
            ++i;

            Block newBlock;
            newBlock.name = name;
            newBlock.args = args;

            blockStack.top()->locationBlock.push_back(newBlock);

            Block* ptr = &blockStack.top()->locationBlock.back();

            blockStack.push(ptr);
        }
        // DIRECTIVE
        else if (i < tokens.size() && tokens[i] == ";")
        {
            ++i;
            blockStack.top()->directives[name].push_back(args);
        }
    }
    return root;
}

// DEBUG:
void printBlock(const Block& block)
{
    std::cout <<  block.name;

    for (const auto& arg : block.args)
        std::cout << " " << arg;

    std::cout << "\n";

    for (const auto& d : block.directives)
    {
        for (const auto& entry : d.second)
        {
            std::cout << d.first << " ";

            for (const auto& val : entry)
                std::cout << val << " ";

            std::cout << "\n";
        }
    }

    for (const auto& child : block.locationBlock)
        printBlock(child);
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

void ConfigParser::setConfigContext(ServerConfig& config, std::string workingBuffer, size_t& pos) {

    // std::map<std::string, std::string>  keys = getContextKeys(workingBuffer);
    // std::map<std::string, std::vector<std::string>>  keys = getContextKeys(workingBuffer);
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

// void ConfigParser::addRoute(const RouteConfig& route)
// {
//     _routes[route.path] = route;
// }
void ConfigParser::buildLocationConfig( const Block& block, ServerConfig& config)
{
    RouteConfig route;

    (void)config;
    // location /images
    if (!block.args.empty())
        route.path = block.args[0];

    if (block.directives.count("root"))
    {
        route.root =
            block.directives.at("root")[0][0];
    }

    if (block.directives.count("index"))
    {
        route.index =
            block.directives.at("index")[0][0];
    }

    if (block.directives.count("autoindex"))
    {
        route.autoIndex =
            block.directives.at("autoindex")[0][0] == "on";
    }

    if (block.directives.count("methods"))
    {
        route.allowedMethods =
            block.directives.at("methods")[0];
    }

    // config.addRoute(route);
}

void ConfigParser::buildServerConfig( const Block& block, ServerConfig& config)
{
    auto it = block.directives.find("listen");

    if (it != block.directives.end() && !it->second.empty())
    {
        int port = std::stoi(it->second[0][0]);
        config.setPort(port);
        std::cout << port << std::endl;
        std::cout << "errorcode" << std::endl;
    }
    else
    {
        std::cerr << "Error: missing 'listen' directive in server block\n";
    }
    // listen
    // int port = std::stoi(block.directives.at("listen")[0][0]);
    // config.setPort(port);
    // std::cout << port << std::endl;
    // std::cout << "errorcode" << std::endl;
    if (block.directives.count("listen"))
    {
    }
    exit(1);
    // host
    if (block.directives.count("host"))
        config.setHost( block.directives.at("host")[0][0]);

    // server_name
    if (block.directives.count("server_name"))
        config.setServerName( block.directives.at("server_name")[0][0]);

    if (block.directives.count("clientMaxBodySize"))
        config.setClientMaxBodySize(std::stoi(block.directives.at("clientMaxBodySize")[0][0]));
    if (block.directives.count("error_page"))
    {
        int errorcode = std::stoi(block.directives.at("error_page")[0][0]);
        std::cout << errorcode << std::endl;
        std::cout << "errorcode" << std::endl;
        printvar(errorcode);
        // config.setErrorPage(block.directives.at("error_page"));
    }
    printconfig(config); // DEBUG:
    exit(3);
    // location blocks
    for (const Block& child : block.locationBlock)
    {
        if (child.name == "location")
            buildLocationConfig(child, config);
    }
}

int ConfigParser::parse(std::string configFile) 
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
        // buffer with everything by append line
        setConfigBuffer(line);
    }
    // workingBuffer now have all the configfile
    const std::string &workingBuffer = getConfigBuffer();

    // std::map<std::string, std::string>  keys = getContextKeys(workingBuffer);
    // std::map<std::string, std::vector<std::string>>  keys = getContextKeys(workingBuffer);
    // std::cout << keys << std::endl;
    Block root = parseConfig(workingBuffer);

    printBlock(root);
    // exit(1);

    // creating the ServerConfig object
    ServerConfig config;
    buildServerConfig(root, config);

    // get blocks
    config.pos = 0; // track position
    while (true) {
        size_t found = workingBuffer.find("server {", config.pos); // server { for validation
        if (found == std::string::npos)
            break;
        setConfigContext(config, workingBuffer, config.pos); 
    }
    // printconfig(config); // DEBUG:
    return 0;
    }
