#include "ConfigParser.hpp"
#include <fstream>      //For ile manipulation
#include <sstream>      //For ile manipulation
#include <vector>
#include <string>
#include <cctype>

ConfigParser::ConfigParser()
{
    std::cout << "ConfigParser constructor called." << std::endl;
}
ConfigParser::~ConfigParser()
{
    std::cout << "ConfigParser destructor called." << std::endl;
}

static int openFile(const std::string& file_path, std::fstream* fstream)
{
    if (fstream->is_open())
        fstream->close();

    fstream->open(file_path.c_str(), std::ios::in);
    if (!fstream->is_open())
    {
        fstream->clear();
        return (1);

    }
    return (0);
}

void    ConfigParser::setConfigBuffer(std::string line)
{
    _configBuffer.append(line);
    _configBuffer.append("\n");
}

std::string ConfigParser::getConfigBuffer()
{
    return(_configBuffer);
}

int ConfigParser::parse(std::string configFile)
{
    // logic of parsing
    // read the config file

    std::fstream fstream;
    if (configFile.empty() || openFile(configFile, &fstream))
    {
        std::cerr << "Config file name is not correct." << std::endl;
        return (0);
    }
    std::string line;

    // std::cout << configFile << std::endl;
    std::ifstream file(configFile);
    while (std::getline(file, line)) //Write everything to our string object
    {
        // buffer with everything by append line
        setConfigBuffer(line);
    }
    std::cout << getConfigBuffer() << std::endl;
    return 0;
}
