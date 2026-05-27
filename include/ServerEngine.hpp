# pragma once

#include <poll.h>
#include "ServerManager.hpp"
#include "Client.hpp"

#define MAX_FDS 100

class ServerEngine {
    private:
        
        const ServerManager& _manager; 

       //runtime registries
        std::map<int, Client> _clients;
        struct pollfd         _fds[MAX_FDS];
        //void acceptNewClient(int master_fd);
        //void readFromClient(int client_fd);

    public:

        // (constructor takes the manager as a parameter)
        // intialize the poll loop to -1 heere
        ServerEngine(const ServerManager& manager);

        // (this is the function that starts the infinite while(true) loop)
        void run(); 
};
