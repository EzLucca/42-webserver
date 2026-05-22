#include "getMethod.hpp"
/*
// 1. Pass Client by reference! No ServerManager needed.
void returnPage(Client& activeClient) 
{
    std::string filepath;
    std::string statusText;

    std::string uriRequest = activeClient.getRequest().getUri();
    std::cout << "Requested URI: " << uriRequest << std::endl; 

    // 2. Grab the direct pointer to the rulebook!
    ServerConfig config = activeClient.getConfig();
    
    // Safety check just in case HERE DO 


    switch (activeClient.getResponse().getStatusCode())
    {
        case 200:
        {

            RouteConfig route = config.getRoute(uriRequest); 
            
            if (route != NULL) {
                // Fetch the root and index from your vectorRoute map
                std::string root = route->vectorRoute.at("root").at(0);
                std::string index = route->vectorRoute.at("index").at(0);
                filepath = root + "/" + index;
            } else {
                filepath = "var/www/default.html"; // Fallback if route not found
            }
            
            statusText = "200 OK";
            break;
        }

        case 404:
            // 3. Ask the config directly for the error page!
            filepath = config.getErrorPage(404);
            statusText = "404 Not Found";
            break;

        case 500:
            filepath = config.getErrorPage(500); 
            // If the user didn't specify a 500 page in the .conf, use a hardcoded default
            if (filepath.empty()) 
                filepath = "var/www/errorpages/500.html";
            statusText = "500 Internal Server Error";
            break;

        default:
            filepath = "var/www/errorpages/default.html";
            statusText = "400 Bad Request";
            break;
    }

    std::ifstream file(filepath.c_str()); // .c_str() needed for C++98 ifstream

    if (!file.is_open())
    {
        std::cerr << "Could not open: " << filepath << std::endl;
        // In the future, this should change the status to 404/500 and recursively call returnPage
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string body = buffer.str();

    // 4. C++98 String conversion for Content-Length
    std::stringstream lengthStream;
    lengthStream << body.size();
    std::string contentLength = lengthStream.str();

    std::string response =
        "HTTP/1.1 " + statusText + "\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + contentLength + "\r\n"
        "\r\n" +
        body;

    // Warning: Direct write() is blocking. We will move this to POLLOUT later!
    write(activeClient.getFd(), response.c_str(), response.size());
}
    */