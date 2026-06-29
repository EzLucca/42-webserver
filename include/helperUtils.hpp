#pragma once

#include <iostream>
#include <filesystem>
#include <ServerConfig.hpp>

class Client ;

void        validatePath(const std::string& filePath);
bool        validateUriPath(Client &activeClient);
size_t      getBodyClient(Client &activeClient);
std::string buildSafeTargetPath(const RouteConfig* route, const std::string& uri);
bool        endsWith(const std::string& fullString, const std::string& ending);

void         validateScript(const std::string &filePath);
