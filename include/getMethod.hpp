#include <iostream>
#include <fstream>      //For ile manipulation
#include <sstream>      //For ile manipulation
#include "Client.hpp"
#include <unistd.h>     // For close(), read(), write()


void    returnPage(Client activeClient, int statuscode);
