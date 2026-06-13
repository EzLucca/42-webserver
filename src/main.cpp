#include <cstring> // For memset()
#include <fcntl.h> // For fcntl() and O_NONBLOCK
#include <fstream> //For ile manipulation
#include <iostream>
#include <netinet/in.h> // For struct sockaddr_in
#include <poll.h>       // For poll() and struct pollfd
#include <string>
#include <sys/socket.h> // For socket(), bind(), listen(), accept()
#include <unistd.h>     // For close(), read(), write()
#include <signal.h>

#include "CgiHandler.hpp"
#include "Client.hpp"
#include "ConfigParser.hpp" // For parsing
#include "HttpException.hpp"
#include "HttpParser.hpp" // For parsing
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerEngine.hpp"
#include "getMethod.hpp"
#include "helperUtils.hpp"

#define MAX_FDS 100


bool validateConfigFile(std::string_view &fileName)
{
    size_t found;

    found = fileName.find(".conf");
    if (found == std::string::npos) {
        std::cerr << "Config file not included.";
        return (false);
    }
    if (found != (fileName.size() - 5)) {
        std::cerr << "Config file name is not correct.";
        return (false);
    }

    return (true);
}

int main(int argc, char **argv)
{

    signal(SIGPIPE, SIG_IGN);

    if (argc != 2) {
        std::cout << "Usage:\n\t./webserv [configuration_file]" << std::endl;
        return (1);
    }
    std::string_view fileName = argv[1];

    try
    {
        if (!validateConfigFile(fileName))
            throw std::invalid_argument("Invalid configuration file.");
    } catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return (1);
    }

    std::string configFile = argv[1];
    ConfigParser config;
    ServerManager manager;

    if (config.parse(configFile, manager))
        exit(1); // TODO: handle errors properly on finish version

    struct pollfd fds[MAX_FDS];

    for (int i = 0; i < MAX_FDS; ++i)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }

    std::map<int, const ServerConfig *> masterSocketRegistry;

    if (!manager.setupMasterSockets(fds, masterSocketRegistry))
    {
        std::cerr << "Failed to setup master sockets" << std::endl;
        return 1;
    }

    manager.printServers();

    ServerEngine engine(fds, masterSocketRegistry);
    engine.run();

    return 0;
}
