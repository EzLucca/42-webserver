# pragma once

#include <iostream>
#include <fstream>      //For ile manipulation
#include <sstream>      //For ile manipulation
#include <unistd.h>     // For close(), read(), write()
#include "Client.hpp"
#include "ServerManager.hpp"

void    returnPage(Client &activeClient);
