#include "ServerEngine.hpp"

#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "getMethod.hpp"

extern volatile sig_atomic_t g_serverRunning;

ServerEngine::ServerEngine(struct pollfd fds[],
        std::map<int, const ServerConfig*>& masterSocketRegistry)
    : _masterSocketRegistry(masterSocketRegistry),
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
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;
    std::cerr << "Accept failed on Master FD " << triggered_fd
        << ". Error: " << strerror(errno) << std::endl;
    exit(1);
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

            switch (activeClient.getState())
            {
                case CGI_IO_OK:
                    _fds[i].events = POLLHUP;
                    break;

                case CGI_IO_DONE:

                    activeClient.getRequest().cleanupBodyFile();
                    activeClient.getResponse().setResponseBody(cgi.output);

                    {
                        size_t headerEnd = cgi.output.find("\r\n\r\n");

                        if (headerEnd != std::string::npos) 
                        {

                            size_t bodySize = cgi.output.length() - (headerEnd + 4); 


                            std::string contentLengthHeader = "Content-Length: " + std::to_string(bodySize) + "\r\n";


                            cgi.output.insert(headerEnd + 2, contentLengthHeader);
                        }
                        std::string final_response =
                            "HTTP/1.1 200 OK\r\n" + cgi.output;
                        activeClient.getResponse().setResponseBuffer(final_response);


                        close(_fds[i].fd);
                        _fds[i].fd = -1;
                        _fdRegistry.erase(triggered_fd);
                        _cgiProcesses.erase(triggered_fd);
                        activeClient.setState(WRITING_RESPONSE);

                    }

                    for (int k = 0; k < MAX_FDS; k++)
                    {
                        if (_fds[k].fd == originalClientFd)
                        {
                            _fds[k].events = POLLOUT;
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
    if (_fds[i].revents & POLLIN)
    {

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
        if (activeClient.getState() != ERROR)
        {
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
        }
        std::cout << "ERROR STATUS CODE: " << activeClient.getResponse().getStatusCode() << std::endl;
        std::cout << "STATUS: " << activeClient.getState() << std::endl;
        if (activeClient.getState() == PROCESSING)
        {

            std::string filename = activeClient.getRequest().getFilename();
            std::string currentMethod = activeClient.getRequest().getMethod();
            bool isCgi = false;

            const ServerConfig *config = activeClient.getConfig();
            const RouteConfig *route = config->getRoute(activeClient.getRequest().getLocationKey());


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

                if (activeClient.getState() != ERROR)
                {

                    std::unordered_map<std::string, std::vector<std::string> >::const_iterator cgiIt = route->vectorRoute.find("cgi_pass");
                    std::unordered_map<std::string, std::vector<std::string> >::const_iterator uploadIt = route->vectorRoute.find("upload_enable");
                    std::cout << filename << " <------------------------FILENAME" << std::endl;

                    std::unordered_map<std::string, std::vector<std::string> >::const_iterator cgiExtIt = route->vectorRoute.find("cgi_ext");
                    if (cgiIt != route->vectorRoute.end())
                    {
                        bool validExtensionFound = false;

                        if (cgiExtIt != route->vectorRoute.end())
                        {
                            std::string allowedExtensions = cgiExtIt->second[0];
                            if (endsWith(filename, allowedExtensions))
                            {
                                validExtensionFound = true;
                            }
                        }
                        if (validExtensionFound)
                        {
                            isCgi = true; // Perfect match, proceed to CGI execution
                        }
                        else
                        {
                            activeClient.getResponse().setStatusCode(403);
                            activeClient.getResponse().setStatusMessage("Forbidden");
                            activeClient.setState(ERROR);
                        }
                    }
                    else if (currentMethod == "POST" && uploadIt != route->vectorRoute.end() && !uploadIt->second.empty() && uploadIt->second[0] == "on")
                    {
                        std::string contentType = activeClient.getRequest().getHeaders()["content-type"];
                        if (contentType.find("multipart/form-data") != std::string::npos || 
                                contentType.find("application/x-www-form-urlencoded") != std::string::npos)
                        {
                            // HTML form hijack
                            isCgi = true;
                            activeClient.getRequest().setFilename("var/cgi/cgi-bin/betterUpload.py");
                            std::cout << "HIJACK ACTIVATED: Routing HTML Form POST to CGI script!" << std::endl;
                        }
                    }


                    if (isCgi)
                    {
                        std::cout << "Valid CGI request detected. Changing state to CGI_CALL." << std::endl;
                        activeClient.setState(CGI_CALL);
                    }
                    else if (activeClient.getState() == PROCESSING && currentMethod == "POST")
                    {
                        if (uploadIt == route->vectorRoute.end() && !uploadIt->second.empty() && uploadIt->second[0] != "on")
                        {
                            // Uploads are not explicitly enabled for this route!
                            activeClient.getResponse().setStatusCode(403);
                            activeClient.getResponse().setStatusMessage("Forbidden");
                            activeClient.setState(ERROR);
                        }
                        else
                        {
                            // RAW UPLOAD LOGIC
                            std::string targetDir = "";
                            std::unordered_map<std::string, std::vector<std::string> >::const_iterator rootIt = route->vectorRoute.find("root");

                            if (rootIt != route->vectorRoute.end() && !rootIt->second.empty()) {
                                targetDir = rootIt->second[0]; 
                            } else {
                                targetDir = "var/www/uploads"; 
                                std::cout << "Warning: No root found in config for upload, using fallback." << std::endl;
                            }

                            std::string tempPath = activeClient.getRequest().getBodyFilePath();
                            std::string uploadFilename = activeClient.getRequest().getFilename();

                            if (uploadFilename.empty() || uploadFilename == "/") {

                                std::stringstream ss;
                                ss << time(NULL);
                                uploadFilename = "dumped_file_" + ss.str(); 
                            }

                            std::string finalPath = targetDir + "/" + uploadFilename; 
                            std::cout << "FINAL PATH: " << finalPath << std::endl; 

                            if (rename(tempPath.c_str(), finalPath.c_str()) == 0)
                            {
                                std::cout << "SUCCESS! File explicitly saved to: " << finalPath << std::endl;
                                activeClient.getRequest().setBodyFilePath("not-set"); 

                                activeClient.getResponse().setStatusCode(201);
                                activeClient.getResponse().setStatusMessage("Created");
                                activeClient.getResponse().setResponseBody("File dumped perfectly via C++!");
                                activeClient.getResponse().buildRawResponse();

                                _fds[i].events = POLLOUT;
                                activeClient.setState(WRITING_RESPONSE);
                            }
                            else
                            {
                                std::cerr << "FATAL: rename() failed! Error: " << strerror(errno) << std::endl;
                                activeClient.getResponse().setStatusCode(500);
                                activeClient.getResponse().setStatusMessage("Internal Server Error");
                                activeClient.setState(ERROR);
                            }

                        }
                    }
                    else if (activeClient.getState() == PROCESSING && currentMethod == "DELETE")
                    {
                        // DELETE LOGIC
                        activeClient.getRequest().handleDeleteRequest(activeClient);
                        activeClient.getRequest().cleanupBodyFile();
                        activeClient.getResponse().setStatusCode(204);
                        activeClient.getResponse().setStatusMessage("No Content");
                        activeClient.getResponse().setResponseBody(""); // 204 has no body!
                        activeClient.getResponse().buildRawResponse();

                        _fds[i].events = POLLOUT;
                        activeClient.setState(WRITING_RESPONSE);
                    }
                    else
                    {
                        if (activeClient.getState() == PROCESSING)
                        {
                            // STATIC GET LOGIC
                            try {
                                std::cout << "Static file request. Calling returnPage." << std::endl;
                                returnPage(activeClient);
                                if (activeClient.getState() == WRITING_RESPONSE)
                                {
                                    _fds[i].events = POLLOUT;
                                }
                            } catch (const std::exception &e) {
                                std::cerr << "Error: " << e.what() << std::endl;
                                activeClient.getResponse().setStatusCode(404);
                                activeClient.getResponse().setStatusMessage("Not Found");
                                activeClient.setState(ERROR);   
                                std::cout << "Client fd: " << activeClient.getFd() << " marked ERROR" << std::endl;
                            }
                        }
                    }

                }
            }
            else
            {
                // Safety catch for missing routes
                activeClient.getResponse().setStatusCode(404);
                activeClient.getResponse().setStatusMessage("Not found");
                activeClient.setState(ERROR);
            }
        }
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
                        throw HttpException(500, "Internal server error");
                        close(cgi.responseFd);
                    }
                }
                else
                {
                    throw HttpException(500, "Internal server error");
                }
            } catch (const HttpException &e) {
                std::cerr << "Error: " << e.what() << std::endl;
                activeClient.setState(ERROR);
                activeClient.getResponse().setStatusCode(e.getStatusCode());
                activeClient.getResponse().setStatusMessage(e.getStatusMessage());
            }
        }
    }
    if (activeClient.getState() == ERROR)
    {
        try {
            std::cout << "Returning page from error block" << std::endl;
            std::cout << "ERROR STATUS CODE: " << activeClient.getResponse().getStatusCode() << std::endl;
            returnErrorPage(activeClient); 
            activeClient.getRequest().cleanupBodyFile();

            activeClient.setState(WRITING_RESPONSE);
            _fds[i].events = POLLOUT;
        } 
        catch (const std::exception &e)
        {
            std::cerr << "Fatal Error building error page: " << e.what() << std::endl;
            activeClient.setState(FINISHED);
        }
    }
    if (activeClient.getState() == FINISHED)
    {
        std::cout << "Process is finished. Dropping connection." << std::endl;

        activeClient.getRequest().cleanupBodyFile();
        _clients.erase(currentFd);
        close(_fds[i].fd);
        _fds[i].fd = -1;
        return ;
    }

    if (_fds[i].revents & POLLOUT)
    {
        std::string& buffer = activeClient.getResponse().getBuffer();
        //std::cout << "DEBUG: POLLOUT triggered. Buffer size: " << buffer.size() 
        //  << " | IsStreaming: " << activeClient.getResponse().isStreaming() << std::endl;

        // pump data 
        if (buffer.empty() && activeClient.getResponse().isStreaming())
        {
            char chunk[8192];
            int fd = activeClient.getResponse().getFileFd();


            // read from fd
            ssize_t bytesRead = read(fd, chunk, sizeof(chunk));

            if (bytesRead > 0)
            {

                buffer.append(chunk, bytesRead);
            }
            else if (bytesRead <= 0)
            {   
                if (bytesRead < 0) {
                    std::cerr << "File stream error!" << std::endl;
                }
                std::cout << "DEBUG: EOF or Error reached, closing FD " << fd << std::endl;
                close(fd);
                activeClient.getResponse().setStreamingFlag(false);
            }
        }

        // send data to browser
        if (!buffer.empty())
        {
            // FIXED: Using currentFd instead of triggered_fd
            ssize_t bytesSent = write(currentFd, buffer.c_str(), buffer.size());

            if (bytesSent > 0) {
                buffer.erase(0, bytesSent); 
            }
            else if (bytesSent < 0) 
            {

                std::cout << "fatal error" << std::endl;
                _clients.erase(currentFd);
                close(_fds[i].fd);
                _fds[i].fd = -1; 
            }
        }
        // reset when fully finished
        //std::cout << "is streaming: " << activeClient.getResponse().isStreaming() << std::endl;
        if (buffer.empty() && !activeClient.getResponse().isStreaming())
        {

            if (activeClient.getRequest().getKeepAlive() == false)
            {
                std::cout << "DEBUG: Streaming complete. closing connection" << std::endl;
                _clients.erase(currentFd);
                close(_fds[i].fd);
                _fds[i].fd = -1; 
            }
            else
            {
                std::cout << "DEBUG: Streaming complete. Resetting client" << std::endl;
                activeClient.resetForNextRequest();
                _fds[i].events = POLLIN; 
            }
        }
    }
}

void ServerEngine::run()
{

    while (g_serverRunning)
    {
        std::cout << "Server engine starting event loop..." << std::endl;
        while (true)
        {
            int poll_count = poll(_fds, MAX_FDS, POLL_TIMEOUT_MS);
            if (poll_count < 0)
            {
                if (!g_serverRunning)
                    break;
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

                if (!(_fds[i].revents & (POLLIN | POLLHUP | POLLOUT)))
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
}
