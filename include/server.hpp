#pragma once

#include <iostream>
#include <string>
#include <sys/socket.h> // For socket(), bind(), listen(), accept()
#include <netinet/in.h> // For struct sockaddr_in
#include <poll.h>       // For poll() and struct pollfd
#include <fcntl.h>      // For fcntl() and O_NONBLOCK
#include <unistd.h>     // For close(), read(), write()
#include <cstring>      // For memset()
#include <fstream>      //For ile manipulation
