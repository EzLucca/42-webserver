#include "ServerEngine.hpp"

#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "getMethod.hpp"

ServerEngine::ServerEngine(const ServerManager& manager,
                           struct pollfd fds[],
                           std::map<int, const ServerConfig*>& masterSocketRegistry)
: _manager(manager),
 _masterSocketRegistry(masterSocketRegistry),
  _httpParser()
{
    for (int i = 0; i < MAX_FDS; ++i)
    {
        _fds[i].fd = fds[i].fd;
        _fds[i].events = fds[i].events;
        _fds[i].revents = 0;
    }
}

void ServerEngine::printFdRegistry() const
{
    std::cout << "----- fdRegistry -----" << std::endl;

    if (_fdRegistry.empty())
    {
        std::cout << "fdRegistry is empty" << std::endl;
        return;
    }

    for (std::map<int, int>::const_iterator it = _fdRegistry.begin();
            it != _fdRegistry.end();
            ++it)
    {
        std::cout << "key(fd): " << it->first
            << " -> value(clientFd): " << it->second
            << std::endl;
    }

    std::cout << "----------------------" << std::endl;
}

bool ServerEngine::isClientWaitingForCgi(int clientFd) const
{
    for (std::map<int, int>::const_iterator it = _fdRegistry.begin(); it != _fdRegistry.end(); ++it)
    {
        if (it->second == clientFd)
            return (true);
    }
    return (false);
}

void ServerEngine::removeFdFromPoll(int fd)
{
    for (int i = 0; i < MAX_FDS; i++)
    {
        if (_fds[i].fd == fd)
        {
            _fds[i].fd = -1;
            _fds[i].events = 0;
            _fds[i].revents = 0;
            return;
        }
    }
}

void ServerEngine::checkClientTimeouts(time_t now)
{
    for (std::map<int, Client>::iterator clientIt = _clients.begin(); clientIt != _clients.end();)
    {
        int clientFd = clientIt->first;
        Client& client = clientIt->second;
        if (isClientWaitingForCgi(clientFd))
        {
            clientIt++;
            continue;
        }
        if (now - client.getLastActivity() > CLIENT_TIMEOUT)
        {
            close(clientFd);
            removeFdFromPoll(clientFd);
            _clients.erase(clientIt++);
        }
        else
        {
            clientIt++;
        }
    }
}

void ServerEngine::checkCgiTimeouts(time_t now)
{
    for (std::map<int, CgiProcess>::iterator cgiIt = _cgiProcesses.begin(); cgiIt != _cgiProcesses.end();)
    {
        int cgiFd = cgiIt->first;
        CgiProcess& cgi = cgiIt->second;

        if (now - cgi.startedAt > CGI_TIMEOUT)
        {
            std::map<int, int>::iterator regIt = _fdRegistry.find(cgiFd);
            int clientFd = -1;
            if (regIt != _fdRegistry.end())
            {
                clientFd = regIt->second;
            }
            if (cgi.pid > 0)
            {
                int status;
                if (kill(cgi.pid, SIGKILL) == -1)
                    std::cerr << "SIGKILL failure" << std::endl;
                if (waitpid(cgi.pid, &status, WNOHANG) == -1)
                    std::cerr << "Child reaping failed" << std::endl;
            }
            close(cgiFd);
            removeFdFromPoll(cgiFd);
            if (clientFd != -1)
            {
                close(clientFd);
                removeFdFromPoll(clientFd);
                std::map<int, Client>::iterator clientIt = _clients.find(clientFd);
                if (clientIt != _clients.end())
                {
                    clientIt->second.getRequest().cleanupBodyFile();
                    _clients.erase(clientIt);
                }
            }
            _fdRegistry.erase(cgiFd);
            _cgiProcesses.erase(cgiIt++);
        }
        else
        {
            cgiIt++;
        }
    }
}

void ServerEngine::acceptNewClient(int triggered_fd)
{
    std::map<int, const ServerConfig *>::const_iterator it = _masterSocketRegistry.find(triggered_fd);
    if (it == _masterSocketRegistry.end())
        return;

    const ServerConfig *matchedConfig = it->second;
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);

    int new_client_fd = accept(triggered_fd, (struct sockaddr *)&client_address, &client_len);
    if (new_client_fd < 0)
    {
        std::cerr << "Accept failed on Master FD " << triggered_fd
            << ". Error: " << strerror(errno) << std::endl;
        exit(1);
    }
    if (new_client_fd == -1)
    {
        std::cerr << "Failure in accepting" << std::endl;
        return;
    }

    fcntl(new_client_fd, F_SETFL, O_NONBLOCK); // set file status flags to nonblocking

    bool added = false; // flag if adding client succesfull

    for (int j = 0; j < MAX_FDS; j++)
    {
        if (_fds[j].fd == -1)
        {
            _fds[j].fd = new_client_fd;
            _fds[j].events = POLLIN; //  activate pollin
            _clients[new_client_fd] = Client(new_client_fd, matchedConfig);

            std::cout << "Link created! Client " << new_client_fd
                << " bound to port " << matchedConfig->getPort()
                << std::endl;
            added = true;
            break;
        }
    }
    if (!added)
    {
        std::cerr << "Server full, rejecting client." << std::endl;
        close(new_client_fd); // close the connection because server full
    }
}

void ServerEngine::handleCgiFd(int i, int triggered_fd)
{
    std::map<int, int>::iterator regIt = _fdRegistry.find(triggered_fd);
    std::map<int, CgiProcess>::iterator cgiIt = _cgiProcesses.find(triggered_fd);

    if (regIt != _fdRegistry.end())
    {
        int originalClientFd = regIt->second;
        std::map<int, Client>::iterator clientIt = _clients.find(originalClientFd);
        if (clientIt == _clients.end())
            return;

        Client &activeClient = clientIt->second;
        if (cgiIt != _cgiProcesses.end())
        {
            CgiProcess &cgi = cgiIt->second;
            activeClient.getResponse().CgiReadResponse(cgi, activeClient);
            std::cout << "It is stuck here7" << std::endl;

            switch (activeClient.getState())
            {
                case CGI_IO_OK:
                    _fds[i].events = POLLHUP;
                    break;

                case CGI_IO_DONE:
                    activeClient.getRequest().cleanupBodyFile();
                    activeClient.getResponse().setResponseBody(cgi.output);

                    std::cout
                        << "\n--- CGI SCRIPT FINISHED! OUTPUT: ---\n"
                        << "CGI OUTPUT: "
                        << cgi.output
                        << std::endl
                        << "\n------------------------------------\n";

                    {
                        std::string final_response =
                            "HTTP/1.1 200 OK\r\n" + cgi.output;

                        write(originalClientFd,
                                final_response.c_str(),
                                final_response.size());
                    }

                    close(_fds[i].fd);
                    _fds[i].fd = -1;

                    _fdRegistry.erase(triggered_fd);
                    _cgiProcesses.erase(triggered_fd);

                    std::cout << "CGI responseFd: "
                        << cgi.responseFd
                        << std::endl;

                    close(cgi.responseFd);

                    activeClient.setState(FINISHED);

                    close(originalClientFd);
                    _clients.erase(originalClientFd);

                    for (int k = 0; k < MAX_FDS; k++)
                    {
                        if (_fds[k].fd == originalClientFd)
                        {
                            _fds[k].fd = -1;
                            break;
                        }
                    }
                    break;

                case CGI_IO_ERROR:
                    activeClient.getRequest().cleanupBodyFile();
                    std::cerr << "CGI Error encountered."
                        << std::endl;

                    close(_fds[i].fd);
                    _fds[i].fd = -1;

                    _fdRegistry.erase(triggered_fd);
                    _cgiProcesses.erase(triggered_fd);
                    _clients.erase(originalClientFd);
                    break;

                default:
                    return;
            }
        }
    }
}

void ServerEngine::handleClientFd(int i, int currentFd)
{
    std::map<int, Client>::iterator clientIt = _clients.find(currentFd);
    if (clientIt == _clients.end())
        return;

    Client &activeClient = clientIt->second;

    char shovelBuffer[8192] = {0}; // intializing buffer with zeros

    int valRead = read(_fds[i].fd, shovelBuffer, sizeof(shovelBuffer));

    if (valRead <= 0)
    {
        close(_fds[i].fd);
        _fds[i].fd = -1;
        _clients.erase(currentFd);
        std::cerr << "Connection dropped out or unidentified error occured."
            << std::endl;
        return;
    }

    activeClient.appendToBuffer(shovelBuffer, valRead); // append the buffer
    activeClient.updateLastActivity();

    try
    {
        _httpParser.parse(activeClient);
    }
    catch (const HttpException &e)
    {
        activeClient.setState(ERROR);
        std::cout << e.getStatusCode() << " <--- statuscode." << std::endl;
        activeClient.getResponse().setStatusCode(e.getStatusCode());
        activeClient.getResponse().setStatusMessage(e.getStatusMessage());
    }

    if (activeClient.getState() == PROCESSING)
    {
        std::cout << "It is stuck here5 with client state: " << activeClient.getState() << std::endl;

        std::string filename = activeClient.getRequest().getFilename();
        std::string currentMethod = activeClient.getRequest().getMethod();
        std::cout << filename << " name script" << std::endl;
        bool isCgi = false;

        const ServerConfig *config = activeClient.getConfig();
        const RouteConfig *route = config->getRoute(activeClient.getRequest().getLocationKey());
        std::cout << "status after validate " << activeClient.getResponse().getStatusCode() << std::endl;

        if (route != NULL)
        {
            bool methodAllowed = false;
            std::unordered_map<std::string, std::vector<std::string>>::const_iterator methodIt = route->vectorRoute.find("allowed_methods");

            if (methodIt != route->vectorRoute.end())
            {
                const std::vector<std::string>& allowedList = methodIt->second;
                for (size_t k = 0; k < allowedList.size(); ++k)
                {
                    if (allowedList[k] == currentMethod)
                    {
                        methodAllowed = true;
                        break;
                    }
                }
            }
            else
            {
                if (currentMethod == "GET") {
                    methodAllowed = true;
                }
            }

            if (!methodAllowed)
            {
                activeClient.getResponse().setStatusCode(405);
                activeClient.getResponse().setStatusMessage("Method no allowed");
                activeClient.setState(ERROR);
            }

            std::unordered_map<std::string, std::vector<std::string>>::const_iterator it2 = route->vectorRoute.find("cgi_pass");

            if (it2 != route->vectorRoute.end() && endsWith(filename, ".py"))
            {
                isCgi = true;
            }

            if (activeClient.getRequest().getMethod() == "POST")
            {
                std::unordered_map<std::string, std::vector<std::string>>::const_iterator uploadIt = route->vectorRoute.find("upload_enable");

                if (uploadIt != route->vectorRoute.end() && !uploadIt->second.empty() && uploadIt->second[0] == "on")
                {
                    isCgi = true;
                    activeClient.getRequest().setFilename("var/cgi/cgi-bin/betterUpload.py");

                    std::cout << "HIJACK ACTIVATED: Routing POST upload to CGI script!" << std::endl;
                }
            }
        }

        if (activeClient.getRequest().getMethod() == "DELETE")
        {
            activeClient.getRequest().handleDeleteRequest(activeClient);
            activeClient.getRequest().cleanupBodyFile();
            activeClient.setState(FINISHED);
        }
        else if (isCgi)
        {
            std::cout << "Valid CGI request detected. Changing state to CGI_CALL." << std::endl;
            activeClient.setState(CGI_CALL);
        }
        else
        {
            try {
                std::cout << "Static file request. Calling returnPage." << std::endl;
                returnPage(activeClient);
            } catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
                activeClient.setState(ERROR);
                std::cout << activeClient.getState() << std::endl;
                std::cout << "Client fd: " << activeClient.getFd()
                    << " marked ERROR"
                    << std::endl;
            }
            if (activeClient.getState() != ERROR)
            {
                activeClient.getRequest().cleanupBodyFile();
                activeClient.setState(FINISHED);
            }
        }

        std::cout << "test" << activeClient.getState() << std::endl;
        if (activeClient.getState() == CGI_CALL)
        {
            std::cout << "\n[DEBUG CGI PRE-FLIGHT CHECK]" << std::endl;
            std::cout << "Original URI: " << activeClient.getRequest().getUri() << std::endl;
            std::cout << "Target Script: " << activeClient.getRequest().getFilename() << std::endl;
            std::cout << "----------------------------\n" << std::endl;
            try {
                CgiHandler CgiObject(activeClient);
                CgiProcess cgi = CgiObject.CgiStart(activeClient.getRequest());

                bool added = false;
                if (cgi.valid == true)
                {
                    for (int j = 0; j < MAX_FDS; j++)
                    {
                        if (_fds[j].fd == -1)
                        {
                            _fds[j].fd = cgi.responseFd;
                            _fds[j].events = POLLIN | POLLHUP;
                            added = true;
                            _fdRegistry.insert(std::make_pair(cgi.responseFd, activeClient.getFd()));
                            _cgiProcesses.insert(std::make_pair(_fds[j].fd, cgi));
                            break;
                        }
                    }
                    if (!added)
                    {
                        std::cerr << "Server full, rejecting CGI process." << std::endl;
                        close(cgi.responseFd);
                    }
                }
            } catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
                activeClient.setState(ERROR);
                std::cout << activeClient.getState() << std::endl;
                std::cout << "Client fd: " << activeClient.getFd()
                    << " marked ERROR"
                    << std::endl;
            }
        }
    }

    std::cout << "2test " << activeClient.getState() << std::endl;
    if (activeClient.getState() == ERROR || activeClient.getState() == FINISHED)
    {
        if (activeClient.getState() == ERROR)
        {
            try {
                std::cout << "Returning page from error block" << std::endl;
                returnErrorPage(activeClient);
            } catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
        activeClient.getRequest().cleanupBodyFile();
        _clients.erase(currentFd);
        close(_fds[i].fd);
        _fds[i].fd = -1;
    }
}

void ServerEngine::run()
{
    std::cout << "Server engine starting event loop..." << std::endl;
    while (true)
    {
        int poll_count = poll(_fds, MAX_FDS, POLL_TIMEOUT_MS);
        if (poll_count < 0)
        {
            std::cerr << "Poll error" << std::endl;
            break;
        }

        time_t now = time(NULL);
        checkClientTimeouts(now);
        checkCgiTimeouts(now);

        for (int i = 0; i < MAX_FDS; i++)
        {
            if (_fds[i].fd == -1)
                continue;

            if (_fds[i].revents != 0)
            {
                std::cout << ">>> POLL WOKE UP! FD: " << _fds[i].fd
                    << " | Revents code: " << _fds[i].revents << " <<<" << std::endl;
            }

            if (!(_fds[i].revents & (POLLIN | POLLHUP)))
            {
                continue;
            }

            int triggered_fd = _fds[i].fd;
            std::map<int, const ServerConfig *>::const_iterator it = _masterSocketRegistry.find(triggered_fd);
            if (it != _masterSocketRegistry.end())
            {
                acceptNewClient(triggered_fd);
                continue;
            }

            if (_fdRegistry.find(triggered_fd) != _fdRegistry.end())
            {
                handleCgiFd(i, triggered_fd);
                continue;
            }

            handleClientFd(i, _fds[i].fd);
        }
    }
}