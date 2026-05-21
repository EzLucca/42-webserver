#include <iostream>
#include <fstream>      //For ile manipulation
#include <sstream>      //For ile manipulation
#include "Client.hpp"
#include "ServerManager.hpp"
#include <unistd.h>     // For close(), read(), write()

void    returnPage(Client activeClient, ServerManager server);
