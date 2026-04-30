#include <iostream>
#include <string>
#include <sys/socket.h> // For socket(), bind(), listen(), accept()
#include <netinet/in.h> // For struct sockaddr_in
#include <poll.h>       // For poll() and struct pollfd
#include <fcntl.h>      // For fcntl() and O_NONBLOCK
#include <unistd.h>     // For close(), read(), write()
#include <cstring>      // For memset()
#include <fstream>      //For ile manipulation

#include "HttpParser.hpp" // For parsing
#include "Client.hpp"
#include "HttpRequest.hpp"

#define PORT 8080
#define MAX_CLIENTS 100

int main() {

    //Lets start parsing the configFile
    //std::string readConf;

    //std::ifstream MyReadFile("configFile.conf"); //Open file
    //while (std::getline(MyReadFile, readConf)) //Write everything to our string object
    //    std::cout << readConf;
    //MyReadFile.close(); //Close the file

    
    // create master socket
    // AF_INET = IPv4, SOCK_STREAM = TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    int opt = 1; // works as 1/0 switch
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) // set socket options
        std::cerr << "setsockopt failed." << std::endl; // we want to add the reuseaddr, so our port is opened staight away after program shuts

    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) //set file status flags to nonblocking
    {
        std::cerr << "fcntl failed." << std::endl;
        return 1;
    }

    // define address and port (bind)
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen all interfaces CHECK THIS
    address.sin_port = htons(PORT);       // hton transforms port to understand the byte order

    // once executed succesfully, os registers that any it traffic that arrives at port 8080
    // must be routed directly to this specific c++ program
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        std::cerr << "Bind failed. Is the port already in use?" << std::endl;
        return 1;
    }
    //with listen we transform default active socket into passice socket (server mode)
    // also initializes queue for in case of client rush. Somaxconn macro gives us largest queue
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        std::cerr << "Listen failed" << std::endl;
        return (1);
    }
    
    //prepare poll struct
    struct pollfd fds[MAX_CLIENTS];
    
    // Initialize, -1 means untouched
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        fds[i].fd = -1; 
    }

    // set master socket in the first index
    fds[0].fd = server_fd;
    fds[0].events = POLLIN; // POLLIN means tell me when there is data to read 

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    std::map<int, Client> clients;
    HttpParser httpParser; // create one http parser for the server

    // Main event loop
    while (true) {
        // poll() waits here, timeout -1 means that it waits infinitely that somethin happpens
        int poll_count = poll(fds, MAX_CLIENTS, -1);
        
        if (poll_count < 0) {
            std::cerr << "Poll error" << std::endl;
            break;
        }

        // go through structs, and see who woke up poll():n
        for (int i = 0; i < MAX_CLIENTS; i++) {
            // did this specific socket actually ring? if not, continue
            if (!(fds[i].revents & POLLIN)) 
                continue;

            // Master socket wokeup, some1 wants to connect, what kind of socket is this?
            if (fds[i].fd == server_fd) 
            {
                // Create empty struct to store client information
                struct sockaddr_in client_address;

                // We need to save the size of the storage
                // type is socklen_t, coz accept() demands this type
                socklen_t client_len = sizeof(client_address);

                // Call accept DOUBLE  CHECK ACCEPT FUNCTION
                int new_client_fd = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
                if (new_client_fd == -1)
                {
                    std::cerr << "Failure in accepting" << std::endl;
                    continue;
                }

                fcntl(new_client_fd, F_SETFL, O_NONBLOCK); //set file status flags to nonblocking
                bool added = false; //flag if adding client succesfull
                

                // Save the client fd, and insert into our array
                for (int j = 0; j < MAX_CLIENTS; j++)
                {
                    if (fds[j].fd == -1)
                    {
                        fds[j].fd = new_client_fd;
                        fds[j].events = POLLIN; //  activate pollin
                        clients.insert(std::make_pair(new_client_fd, Client(new_client_fd))); // add the client to the map
                        added = true;
                        
                        std::cout << "New client connected on FD: "<< new_client_fd << std::endl;
                        break;
                    }
                }
                if (!added) 
                {
                    std::cerr << "Server full, rejecting client." << std::endl;
                    close(new_client_fd); // close the connection because server full
                }
            } 
            // Already existing client woke up and sent us data
            else {

                int currentFd = fds[i].fd; // take the fd who called, this is our key
                Client& activeClient = clients[currentFd]; // get the activeclient

                // 8Kb is standardized  size for single read 
                char shovelBuffer[1] = {0}; //intializing buffer with zeros

                // read data to the buffer 
                int valRead = read(fds[i].fd, shovelBuffer, sizeof(shovelBuffer)); 

                if (valRead <= 0)
                {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    clients.erase(currentFd);
                    std::cerr << "Connection dropped out or unidentified error occured." << std::endl;
                    continue;
                }
                else
                {
                    activeClient.appendToBuffer(shovelBuffer, valRead); // append the buffer
                    httpParser.parse(activeClient);
                    // if parse is completed so if state is processing we start to execute the request, and after that make the response, check if still something in buffer, if, then clear request object and call parse again, make a loop
                }
                // Print the buffuer to the output stream
                //std::cout << shovelBuffer << std::endl;

                /*
                // Hardcoded mock response
                std::string mock_response = 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 13\r\n"
                    "\r\n"
                    "Hello, World!";
                
                
                //lets use write or send to send the mock response to the client
                int bytesSent = write(fds[i].fd, mock_response.c_str(), mock_response.length());
                
                if (bytesSent < 0)
                {
                    std::cerr << "Failed to send response" << std::endl;
                }
                */
                //close the connections, and set the fd back to -1
                if (activeClient.getState() == PROCESSING)
                {
                clients.erase(currentFd); // DUNNO IF THIS WORKS
                close(fds[i].fd);
                fds[i].fd = -1;
                }
            }
        }
    }
    return 0;
}