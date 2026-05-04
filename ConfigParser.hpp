#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <iostream>


// Reads the text file, runs tokenization state machine, and spits out seerverconfig objects
class ConfigParser
{
    private:
        std::string _configBuffer;
    public:
        ConfigParser();
        ~ConfigParser();
        int parse(std::string configFile);
        void    setConfigBuffer(std::string line);
        std::string getConfigBuffer();

};

#endif
