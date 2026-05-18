#include "ConfigParser.hpp"

/**
 * @brief Checks if a file or directory exists.
 *
 * This function uses std::filesystem to verify whether the given file path
 * exists. If the path is invalid, it prints an error message and terminates
 * the program.
 *
 * @param filePath Path to the file or directory to check.
 */
void validatePath(const std::string& filePath)
{
    if (!std::filesystem::exists(filePath))
        throw std::invalid_argument("path does not exist: " + filePath);
}

/**
 * @brief Validates and converts the configured client body size.
 *
 * Reads the value associated with the `client_max_body_size` key
 * from the provided map and converts it into bytes.
 *
 * Supported formats:
 * - Plain integer (e.g. `"1024"`) interpreted as bytes
 * - Integer followed by `'M'` (e.g. `"10M"`) interpreted as megabytes
 *
 * If the key is missing or the value is empty, the function returns `0`.
 *
 * @param values Map containing configuration key-value pairs.
 * @return int Body size in bytes.
 *
 * @throws std::invalid_argument If the numeric portion cannot be parsed.
 * @throws std::out_of_range If the parsed value exceeds integer limits.
 */
int validateBodySize(const std::map<std::string, std::string>& values)
{
    auto it = values.find("client_max_body_size");
    if (it == values.end() || it->second.empty())
        return 0;

    std::string val = it->second;

    if (val.back() == 'M')
    {
        int size = std::stoi(val.substr(0, val.size() - 1));
        return size * 1024 * 1024;
    }
    return std::stoi(val);
}

/**
 * @brief Validates an IPv4 host address.
 *
 * Uses `inet_pton()` to verify that the provided string is a valid
 * IPv4 address.
 *
 * @param hostValue Host address string to validate.
 * @return std::string The validated host string.
 *
 * @throws std::invalid_argument If the host string is not a valid IPv4 address.
 */
std::string validateHost(std::string hostValue)
{
    struct sockaddr_in sa;

    if(inet_pton(AF_INET, hostValue.c_str(), &(sa.sin_addr)) != 1)
        throw std::invalid_argument("host invalid");
    return hostValue;
}

/**
 * @brief Validates and parses the server listen port.
 *
 * Ensures that:
 * - The `listen` key exists in the configuration map
 * - The value is a valid numeric string
 * - The entire string is numeric
 * - The port falls within the allowed range (1024–49151)
 *
 * @param input Map containing configuration key-value pairs.
 * @return int Parsed port number.
 *
 * @throws std::invalid_argument If the `listen` key is missing,
 *         contains non-numeric characters, or input is empty.
 * @throws std::out_of_range If the port is outside the allowed range.
 */
int validatePort(std::map<std::string, std::string> &input)
{
    if (input.find("listen") == input.end())
        throw std::invalid_argument("missing listen key");
    if (input.empty())
        throw std::invalid_argument("port is empty");

    size_t idx = 0;
    int port = 0;

    // check for "a8080"
    port = std::stoi(input["listen"], &idx);

    // Ensure full string was consumed (no "8080abc")
    if (idx != input["listen"].size())
        throw std::invalid_argument("port contains invalid characters");

    // Range check (typical user-space ports)
    if (port < 1024 || port > 49151)
        throw std::out_of_range("port out of allowed range (1024–49151)");

    return port;
}

/**
 * @brief Applies parsed configuration values to a ServerConfig object.
 *
 * This function validates and assigns global server configuration directives
 * such as port, host, server name, and client max body size.
 *
 * Basic validation is performed:
 * - Port must be within valid range (1024–49151)
 * - Host must be a valid IPv4 address
 * - Client max body size is parsed from string
 *
 * @param config Reference to the ServerConfig to populate.
 * @param values Map containing raw configuration key-value pairs.
 */
void    setDirectives(ServerConfig &config, std::map<std::string, std::string> &values)
{
    try
    {
        int port = validatePort(values);
        config.setPort(port);

        std::string host = validateHost(values["host"]);
        config.setHost(host);

        auto nameIt = values.find("server_name");
        if (nameIt == values.end() || nameIt->second.empty())
            throw std::invalid_argument("missing server_name");
        config.setServerName(values["server_name"]);

        int ClientMaxBodySize = validateBodySize(values);
        config.setClientMaxBodySize(ClientMaxBodySize);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Config error: " << e.what() << std::endl;
        std::exit(2);
    }
}

/**
 * @brief Removes leading and trailing whitespace characters from a string.
 *
 * Whitespace includes spaces, tabs, carriage returns, and newlines.
 *
 * @param str Input string to trim.
 * @return std::string Trimmed string.
 */
std::string ConfigParser::trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");

    if (start == std::string::npos)
        return "";

    return str.substr(start, end - start + 1);
}

/**
 * @brief Parses a location block inside the configuration file.
 *
 * This function reads directives inside a location block until it encounters
 * a closing brace ('}'). Each directive is parsed into key-value pairs and
 * stored in the RouteConfig structure.
 *
 * Special handling:
 * - allowed_methods are split into multiple values
 * - trailing semicolons are removed
 *
 * @param stream Input stream positioned inside a location block.
 * @param config RouteConfig object to populate with parsed values.
 */
void ConfigParser::parselocation(std::istream &stream, RouteConfig &nestedLocation, ServerConfig &config)
{
    std::string key;
    std::string value;

    while (stream >> key)
    {
        if (key == "}"|| key == "location")
        {
            if (key == "}")
                config.pos--;
            break;
        }

        std::getline(stream, value);

        value = trim(value);

        if (!value.empty() && value.back() == ';')
            value.pop_back();

        if (key == "root" || key == "cgi_pass")
        {
            validatePath(value);
        }
        if (key == "allowed_methods")
        {
            std::stringstream ss(value);
            std::string word;

            while (ss >> word)
                nestedLocation.vectorRoute[key].push_back(word);
            continue;
        }
        nestedLocation.vectorRoute[key].push_back(value);
    }
}

/**
 * @brief Determines whether a configuration line should be ignored.
 *
 * Currently, only empty lines are considered skippable.
 *
 * @param line Input line from configuration file.
 * @return true if the line should be ignored, false otherwise.
 */
bool ConfigParser::shouldSkipLine(const std::string& line)
{
    return line.empty();
}

/**
 * @brief Checks whether a line represents the end of a block.
 *
 * The function detects closing braces used in configuration syntax.
 *
 * @param line Input line.
 * @return true if the line is a block terminator ("}"), false otherwise.
 */
bool ConfigParser::isBlockEnd(const std::string& line)
{
    return line == "}";
}

/**
 * @brief Checks whether a line marks the start of a server block.
 *
 * Detects the keyword that begins a server configuration block.
 *
 * @param line Input line.
 * @return true if the line is "server {", false otherwise.
 */
bool ConfigParser::isServerStart(const std::string& line)
{
    return line == "server {";
}

/**
 * @brief Parses an error_page directive and stores it in the server config.
 *
 * This function extracts an HTTP error code and its associated file path
 * from the provided directive value. It validates that the error page path
 * exists before assigning it to the ServerConfig object.
 *
 * @param value  String containing the error code and file path.
 * @param config ServerConfig object where the error page mapping is stored.
 */
void ConfigParser::parseErrorPage( const std::string& value, ServerConfig& config)
{
    std::istringstream iss(value);

    int errorCode;
    std::string errorPath;

    iss >> errorCode >> errorPath;

    validatePath(errorPath);
    config.setErrorPage(errorCode, errorPath);
}

/**
 * @brief Parses a location block from the configuration file.
 *
 * Extracts the location path from the provided line, validates the
 * presence of the opening brace `{`, and parses the nested location
 * directives into a RouteConfig object.
 *
 * The parsed route configuration is then stored in the provided
 * ServerConfig instance.
 *
 * @param value  Raw location declaration line containing the route path
 *               and opening brace.
 * @param stream Input stream used to continue parsing nested directives.
 * @param config Server configuration object receiving the parsed route.
 *
 * @throws std::runtime_error If the opening brace `{` is missing.
 */
void ConfigParser::parseLocationBlock( const std::string& value, std::istream& stream, ServerConfig& config)
{
    RouteConfig nestedLocation;

    if (value.find("{") == std::string::npos)
        throw std::runtime_error("{ location not found");
    config.pos++;

    size_t locationPos = value.find(" ");

    std::string locationValue = value.substr(0, locationPos);

    parselocation(stream, nestedLocation, config);

    config.setRoute(locationValue, nestedLocation);
}

/**
 * @brief Parses a single configuration directive line.
 *
 * This function extracts a key-value pair from a configuration line and
 * processes it according to the directive type.
 *
 * - If the directive is `error_page`, it is forwarded to `parseErrorPage()`.
 * - If the directive is `location`, it triggers parsing of a nested location block.
 * - Otherwise, the key-value pair is stored in the provided `values` map
 *   for later assignment to the ServerConfig object.
 *
 * The function also trims whitespace and removes trailing semicolons
 * from directive values.
 *
 * @param line    A single line from the configuration file.
 * @param stream  Input stream (used for parsing nested blocks like location).
 * @param config  ServerConfig object being populated.
 * @param values  Temporary storage for simple key-value directives.
 */
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

/**
 * @brief Parses a server configuration block from an input stream.
 *
 * This function reads lines from the provided input stream and processes
 * them to extract server configuration directives. It skips irrelevant
 * lines, detects the beginning and end of a server block, and delegates
 * directive parsing to helper functions.
 *
 * Parsed directive values are stored in a temporary map and later applied
 * to the provided ServerConfig object.
 *
 * @param config Reference to the ServerConfig object to be filled.
 * @param stream Input stream containing the configuration file data.
 *
 * @return true if a valid server block was found and successfully parsed,
 *         false if no server block was detected.
 */
bool ConfigParser::parseConfig(ServerConfig& config, std::istream& stream)
{
    std::map<std::string, std::string> values;

    std::string line;
    bool foundServer = false;
    config.pos = 0;

    while (std::getline(stream, line))
    {
        line = trim(line);

        if (shouldSkipLine(line))
            continue;
        if (isBlockEnd(line))
        {
            config.pos--;
            break;
        }
        if (isServerStart(line))
        {
            config.pos++;
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

/**
 * @param file_path contain the path of the config file
 * @param fstream file stream
 * Return a int 
 */
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

/**
 * @param configFile contain the path of the config file
 * @param server The main object to contain all the servers objects
 * Return a int 
 */
int ConfigParser::parse(std::string configFile, ServerManager& server) 
{
    // TODO: check if file is open and permissions
    std::fstream fstream;
    if (configFile.empty() || openFile(configFile, &fstream)) {
        // throw std::invalid_argument("Config file error");
        std::cerr << "Config file name is not correct." << std::endl;
        return (1);
    }
    std::string line;

    std::ifstream file(configFile);
    while (std::getline(file, line))
    {
        size_t pos = line.find("#");
        if (pos != std::string::npos)
        {
            line = line.substr(0, pos);
        }
        setConfigBuffer(line);
    }
    const std::string &workingBuffer = getConfigBuffer();

    // TEST:
    try{
        std::istringstream stream(workingBuffer);

        // while (stream)
        while (true)
        {
            ServerConfig config;

            if(!parseConfig(config, stream))
            {
                if (config.pos != 0)
                    throw std::runtime_error("Brackets unclosed.");
                break;
            }
            if (config.pos != 0)
                throw std::runtime_error("Brackets unclosed.");
            server.addServer(config);
        }
        // server.printServers(); // TEST:
    }
    catch (const std::exception& e) 
    {
        std::cout << e.what() << " (testing)" << std::endl;
    }
    // ~TEST:

    return 0;
}
