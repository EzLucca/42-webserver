#pragma once

#include <iostream>
#include <filesystem>

class Client ;

void    validatePath(const std::string& filePath);
bool    validateUriPath(Client &activeClient);
size_t  getBodyClient(Client &activeClient);
