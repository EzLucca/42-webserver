#include <cstring> // For memset()
#include <fcntl.h> // For fcntl() and O_NONBLOCK
#include <fstream> //For ile manipulation
#include <iostream>
#include <netinet/in.h> // For struct sockaddr_in
#include <poll.h>       // For poll() and struct pollfd
#include <string>
#include <sys/socket.h> // For socket(), bind(), listen(), accept()
#include <unistd.h>     // For close(), read(), write()

#include "CgiHandler.hpp"
#include "Client.hpp"
#include "ConfigParser.hpp" // For parsing
#include "HttpException.hpp"
#include "HttpParser.hpp" // For parsing
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "getMethod.hpp"

#define PORT 8080
#define MAX_FDS 100

bool validateConfigFile(std::string_view &fileName) {
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

int main(int argc, char **argv) {

    if (argc != 2) {
        std::cout << "Usage:\n\t./webserv [configuration_file]" << std::endl;
        return (1);
    }
    std::string_view fileName = argv[1];

    try {
        if (!validateConfigFile(fileName))
            throw std::invalid_argument("Invalid configuration file.");
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return (1);
    }

    std::string configFile;
    ConfigParser config;
    ServerManager manager;
    std::map<int, Client> clients;
    HttpParser httpParser; // create one http parser for the server
    std::map<int, CgiProcess> cgiProcesses;
    std::map<int, int> fdRegistry;

    configFile = argv[1];

    if (config.parse(configFile, manager))
        exit(1); // TODO: handle errors properly on finish version

    // catch all the servers.
    const std::vector<ServerConfig> &allServers = manager.getServers();
    std::map<int, const ServerConfig *> masterSocketRegistry;

    // prepare poll struct
    struct pollfd fds[MAX_FDS];

    // Initialize, -1 means untouched
    for (int i = 0; i < MAX_FDS; ++i) {
        fds[i].fd = -1;
    }

    // make loop here and go through all servers and set up the networks
    for (int i = 0; i < manager.getServerCount(); i++) {
        std::cout << "Setting up Master Socket for port: "
            << allServers[i].getPort() << std::endl;

        // create master socket
        // AF_INET = IPv4, SOCK_STREAM = TCP
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return 1;
        }

        int opt = 1; // works as 1/0 switch
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
                0) // set socket options
            std::cerr << "setsockopt failed."
                << std::endl; // we want to add the reuseaddr, so our port is
                              // opened staight away after program shuts

        if (fcntl(server_fd, F_SETFL, O_NONBLOCK) <
                0) // set file status flags to nonblocking
        {
            std::cerr << "fcntl failed." << std::endl;
            return 1;
        }

        // define address and port (bind)
        struct sockaddr_in address;
        std::memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY; // Listen all interfaces CHECK THIS
        address.sin_port = htons(
                allServers[i]
                .getPort()); // hardcoded
                             // must be routed directly to this specific c++ program
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            std::cerr << "Bind failed. Is the port already in use?" << std::endl;
            return 1;
        }
        // with listen we transform default active socket into passice socket
        // (server mode)
        //  also initializes queue for in case of client rush. Somaxconn macro gives
        //  us largest queue
        if (listen(server_fd, SOMAXCONN) < 0) {
            std::cerr << "Listen failed" << std::endl;
            return (1);
        }

        // set master socket in the first index
        fds[i].fd = server_fd;
        fds[i].events = POLLIN; // POLLIN means tell me when there is data to read
        masterSocketRegistry[server_fd] = &allServers[i];
    }
    manager.printServers();

    // exit(2);

    std::cout << "Server listening on port " << PORT << "..." << std::endl;
    // std::cout << "Server listening on port " <<
    // manager.getServerValues("mysite.com", "listen") << "..." << std::endl;
    //  Main event loop
    while (true) {
        // poll() waits here, timeout -1 means that it waits infinitely that
        // somethin happpens

        int poll_count = poll(fds, MAX_FDS, -1);
        if (poll_count < 0) {
            std::cerr << "Poll error" << std::endl;
            break;
        }
        // go through structs, and see who woke up poll():n
        for (int i = 0; i < MAX_FDS; i++) {
            // did this specific socket actually ring? if not, continue
            if (!(fds[i].revents & POLLIN))
                continue;
            int triggered_fd = fds[i].fd;

            // Master socket wokeup, some1 wants to connect, what kind of socket is
            // this?
            std::map<int, const ServerConfig *>::iterator it =
                masterSocketRegistry.find(triggered_fd);
            if (it != masterSocketRegistry.end()) {

                const ServerConfig *matchedConfig = it->second;
                struct sockaddr_in client_address;
                socklen_t client_len = sizeof(client_address);

                // Call accept DOUBLE  CHECK ACCEPT FUNCTION
                int new_client_fd = accept(
                        triggered_fd, (struct sockaddr *)&client_address, &client_len);
                if (new_client_fd < 0) {
                    std::cerr << "Accept failed on Master FD " << triggered_fd
                        << ". Error: " << strerror(errno) << std::endl;
                    exit(1);
                }
                if (new_client_fd == -1) {
                    std::cerr << "Failure in accepting" << std::endl;
                    break;
                }

                fcntl(new_client_fd, F_SETFL,
                        O_NONBLOCK); // set file status flags to nonblocking

                bool added = false; // flag if adding client succesfull

                // Save the client fd, and insert into our array
                for (int j = 0; j < MAX_FDS; j++) {
                    if (fds[j].fd == -1) {
                        fds[j].fd = new_client_fd;
                        fds[j].events = POLLIN; //  activate pollin
                        clients[new_client_fd] = Client(new_client_fd, matchedConfig);

                        std::cout << "Link created! Client " << new_client_fd
                            << " bound to port " << matchedConfig->getPort()
                            << std::endl;
                        added = true;
                        break;
                    }
                }
                if (!added) {
                    std::cerr << "Server full, rejecting client." << std::endl;
                    close(new_client_fd); // close the connection because server full
                }
                continue;
            }
            std::map<int, int>::iterator shit = fdRegistry.find(triggered_fd);
            std::map<int, CgiProcess>::iterator cgiIt =
                cgiProcesses.find(triggered_fd);

            if (shit != fdRegistry.end()) {
                //	int	cgiPipeFd = it->first;
                int originalClientFd = shit->second;
                Client &activeClient = clients[originalClientFd];
                if (cgiIt != cgiProcesses.end()) {
                    CgiProcess cgi = cgiIt->second;
                    activeClient.getResponse().CgiReadResponse(cgi, activeClient);
                    switch (activeClient.getState()) {
                        case CGI_IO_OK: {
                                            continue;
                                        }
                        case CGI_IO_DONE: {
                                              activeClient.getResponse().setResponseBody(cgi.output);
                                              // cleanup cgi object from cgiProcesses
                                              // cleanup cgi fd from fdRegistry and poll loop
                                              // destroy the temp file
                                          }
                        case CGI_IO_ERROR: {
                                               // error handling
                                               // cleanup cgi object from cgiProcesses
                                               // cleanup cgi fd from fdRegistry and poll loop
                                               // destroy the temp file
                                           }
                        default: {
                                     continue;
                                 }
                    }
                }
            }
            // Already existing
            else {

                int currentFd = fds[i].fd; // take the fd who called, this is our key
                Client &activeClient = clients[currentFd]; // get the activeclient

                // 8Kb is standardized  size for single read
                char shovelBuffer[1] = {0}; // intializing buffer with zeros

                // read data to the buffer
                int valRead = read(fds[i].fd, shovelBuffer, sizeof(shovelBuffer));

                if (valRead <= 0) {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    clients.erase(currentFd);
                    std::cerr << "Connection dropped out or unidentified error occured."
                        << std::endl;
                    continue;
                }

                activeClient.appendToBuffer(shovelBuffer, valRead); // append the buffer

                try {
                    httpParser.parse(activeClient);
                }

                catch (const HttpException &e) {
                    activeClient.setState(ERROR);
                    std::cout << e.getStatusCode() << " <--- statuscode. (testing)";
                    activeClient.getResponse().setStatusCode(e.getStatusCode());
                    activeClient.getResponse().setStatusMessage(e.getStatusMessage());
                }

                // if parse is completed so if state is processing we start to execute
                // the request
                if (activeClient.getState() == PROCESSING) {
                    // check for request method
                    //  redirections for methods

                    activeClient.getResponse().setStatusCode(200);
                    // activeClient.getResponse().setStatusCode(404);

                    // TEST:

                    std::string uriRequest = activeClient.getRequest().getUri();
                    std::cout << "Requested URI: " << uriRequest << std::endl;

                    // 2. Grab the direct pointer to the rulebook!
                    const ServerConfig *activeConfig = activeClient.getConfig();

                    std::vector<std::string> klist = activeConfig->getLocationList();
                    activeClient.getRequest().setQueryString();
                    
                    std::cout << activeClient.getRequest().getUriPath() << std::endl;
                    std::cout << activeClient.getRequest().getQueryString() << std::endl;

                    std::string objstring = activeClient.getRequest().getUriPath();
                    size_t finalpos = 0;
                    for (const std::string &s : klist) {
                        size_t pos = activeClient.getRequest().getUriPath().find(s);
                        if(pos != std::string::npos)
                        {
                            pos = s.size();
                            if(pos > finalpos)
                                finalpos = pos;
                            std::cout << finalpos << std::endl;
                        }

                    }
                    activeClient.getRequest().setUriPath(objstring.substr(0, finalpos));
                    activeClient.getRequest().setFilename(objstring.substr(finalpos));

                    if (activeClient.getRequest().getFilename() != "")
                    {
                        activeClient.getRequest().setFilename(objstring.substr(finalpos + 1));
                    }

                    std::cout << activeClient.getRequest().getUriPath() << std::endl;
                    std::cout << activeClient.getRequest().getFilename() << std::endl;
                    // if (uriRequest == s)
                    // {
                    //     returnPage(activeClient);
                    // }
                    // else
                    // {
                    //     //default fallback
                    // }
                    if (activeClient.getRequest().getMethod() == "POST") {
                        std::cout << activeClient.getRequest().getMethod() << std::endl;
                    }

                    // here we process the request and build response on the fly
                    //  if (activeClient.getState() == CGI_CALL)
                    //  {
                    //  	CgiHandler	CgiObject(activeClient.getRequest(),
                    //  activeClient.getConfig()); 	CgiProcess	cgi =
                    //  CgiObject.CgiStart(activeClient.getRequest()); 	bool	added =
                    //  false; 	if (cgi.valid == true)
                    //  	{
                    //  		for (int j = 0; j < MAX_FDS; j++)
                    //  		{
                    //  			if (fds[j].fd == -1)
                    //  			{
                    //  				fds[j].fd = cgi.responseFd;
                    //  				fds[j].events = POLLIN; //  activate
                    //  pollin 				added = true;
                    //  				fdRegistry.insert(std::make_pair(cgi.responseFd,
                    //  activeClient.getFd()));
                    //  				cgiProcesses.insert(std::make_pair(fds[j].fd,
                    //  cgi)); 				break;
                    //  			}
                    //  			if (!added)
                    //  			{
                    //  				std::cerr << "Server full, rejecting CGI
                    //  process." << std::endl; 				close(cgi.responseFd); // close the
                    //  connection because server full 				fds[j].fd = -1;
                    //  			}
                    //  		}
                    //  	}
                    //
                    //  	// CgiHandler(activeClient.getRequest(), server);
                    //
                    //  	//****************************************************************
                    //
                    //  	// after processing and after sending the response, check the
                    //  buffer, if another request, start the loop again
                    //  }
                }
                // Print the buffuer to the output stream
                // std::cout << shovelBuffer << std::endl;

                /*
                // Hardcoded mock response
                std::string mock_response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 13\r\n"
                "\r\n"
                "Hello, World!";


                //lets use write or send to send the mock response to the client
                int bytesSent = write(fds[i].fd, mock_response.c_str(),
                mock_response.length());

                if (bytesSent < 0)
                {
                std::cerr << "Failed to send response" << std::endl;
                }
                */
                // close the connections, and set the fd back to -1
                if (activeClient.getState() == PROCESSING ||
                        activeClient.getState() == ERROR) {

                    clients.erase(currentFd);
                    close(fds[i].fd);
                    fds[i].fd = -1;
                }
            }
        }
    }
    return 0;
}
