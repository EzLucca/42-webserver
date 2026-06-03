#pragma once

#include <iostream>
#include <filesystem>

class Client ;

void    validatePath(const std::string& filePath);
void    validateUriPath(Client &activeClient);
