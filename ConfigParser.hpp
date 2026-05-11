#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <iostream>
#include "ServerConfig.hpp"
#include "ServerManager.hpp"

// Reads the text file, runs tokenization state machine, and spits out seerverconfig objects
class ConfigParser
{
    private:
        std::string _configBuffer;
        RouteConfig _routes;

        // auto    findConfigKey(std::string key,std::string workingBuffer);
        template <typename T>
            T findConfigKey(const std::string& key, const std::string& workingBuffer, size_t& startPos) {
                size_t pos = workingBuffer.find(key, startPos);
                if (pos == std::string::npos) throw std::runtime_error("Key not found");

                pos += key.size();

                while (pos < workingBuffer.size() && isspace(workingBuffer[pos]))
                    pos++;

                size_t endPos = pos;
                while (endPos < workingBuffer.size() && workingBuffer[endPos] != '{' && /*!isspace(workingBuffer[endPos]) &&*/
                        workingBuffer[endPos] != ';' && workingBuffer[endPos] != '#') {
                    endPos++;
                }

                std::string valueStr = workingBuffer.substr(pos, endPos - pos);

                startPos = endPos; // update starting position for next search

                if constexpr (std::is_same_v<T, std::string>) return valueStr;
                else if constexpr (std::is_same_v<T, int>) {
                    // if(valueStr[0] == '/')
                    //     return std::stoi(valueStr.substr(1));
                    return std::stoi(valueStr);
                }
                else if constexpr (std::is_same_v<T, size_t>) return static_cast<size_t>(std::stoul(valueStr));
                else if constexpr (std::is_same_v<T, long>) return std::stol(valueStr);
                else static_assert(sizeof(T) == 0, "Unsupported type");
            }
    public:
        ConfigParser();
        ~ConfigParser();
        // int         parse(std::string configFile, ServerManager& server);
        int         parse(std::string configFile, ServerConfig& server);
        void        setConfigBuffer(std::string line);
        // void        setConfigContext();
        void        setConfigContext(ServerConfig& config, std::string workingBuffer, size_t& pos);
        void        setConfigLocations(ServerConfig& config, std::string workingBuffer, size_t& pos);
        // void        setConfigCgi(ServerConfig& config, std::string workingBuffer, size_t& pos);
        void        addRoute(const RouteConfig& route);

        std::string getConfigBuffer();

};

#endif
