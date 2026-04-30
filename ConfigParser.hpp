#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <iostream>


// Reads the text file, runs tokenization state machine, and spits out seerverconfig objects
class ConfigParser
{
    private:
    public:
        ConfigParser();
        ~ConfigParser();
        void parse(std::string configFile);
};

#endif
