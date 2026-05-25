#include "ServerEngine.hpp"


ServerEngine::ServerEngine(const ServerManager& manager) :
_manager(manager)
{
    // Initializing the poll loop
    for (int i = 0; i < MAX_FDS; ++i) 
    {
        _fds[i].fd = -1; 
    }

    
}

void ServerEngine::run()
{
    // the whole main event loop here 
}