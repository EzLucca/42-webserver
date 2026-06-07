#include "getMethod.hpp"

bool    validateMethod(const RouteConfig *routeLocation, std::string method)
{
    std::unordered_map<std::string, std::vector<std::string>>::const_iterator it = routeLocation->vectorRoute.find("allowed_methods");

    std::cout << "validateMethod function" << std::endl;
    std::cout << method << std::endl;

    if (it == routeLocation->vectorRoute.end())
    {
        std::cout << "allowed_methods not found" << std::endl;
        return false;
    }
    const std::vector<std::string> methods = it->second;
    for (std::vector<std::string>::const_iterator mit = methods.begin();
            mit != methods.end();
            ++mit)
    {
        std::cout << "Allowed method: [" << *mit << "]" << std::endl;
        if (*mit == method)
            return true;
    }
    std::cout << "return end here" << std::endl;
    return false;
}

std::string createResponse(Client& activeClient, std::string filepath)
{
    int code = activeClient.getResponse().getStatusCode();
    std::string statusCode = std::to_string(code);
    std::string statusmsg = activeClient.getResponse().getStatusMessage();
    std::string contentType = activeClient.getResponse().getMimeType(filepath);
    std::ifstream file(filepath.c_str(), std::ios::binary); // NEED TO OPEN IN BINARY MODE
    std::cout << "content type: " << contentType << " | filepath: " << filepath << std::endl;
    if (!file.is_open())
    {
        std::cerr << "Could not open: " << filepath << std::endl;
        // In the future, this should change the status to 404/500 and recursively call returnPage
        activeClient.getResponse().setStatusCode(404);
        returnPage(activeClient);
        activeClient.setState(FINISHED);
        return filepath;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string body = buffer.str();

    activeClient.getResponse().setResponseBody(body);
    // 4. C++98 String conversion for Content-Length
    std::stringstream lengthStream;
    lengthStream << body.size();
    std::string contentLength = lengthStream.str();

    std::string response =
        "HTTP/1.1 " + statusCode + " " + statusmsg + "\r\n"
        "Content-Type: " + contentType + "\r\n"
        "Content-Length: " + contentLength + "\r\n"
        "\r\n" +
        activeClient.getResponse().getResponseBody();
    return response;
}

void    returnErrorPage(Client& activeClient) 
{
    std::string filepath;
    std::string statusCode;
    std::string response;

    std::string uriRequest = activeClient.getRequest().getUri();
    std::cout << "URI from inside returnErrorPage: " << uriRequest << std::endl; 
    std::cout << "ERROR STATUS CODE: " << activeClient.getResponse().getStatusCode() << std::endl;
    statusCode = activeClient.getResponse().getStatusCode();

    const ServerConfig *config = activeClient.getConfig();
    int code = activeClient.getResponse().getStatusCode();

    filepath = config->getErrorPage(code);

    response = createResponse(activeClient, filepath);

    // Warning: Direct write() is blocking. We will move this to POLLOUT later!
    write(activeClient.getFd(), response.c_str(), response.size());
}

void    returnPage(Client& activeClient) 
{
    std::string filepath;
    std::string locationKey = activeClient.getRequest().getLocationKey();
    std::string uriRequest = activeClient.getRequest().getUri();
    std::string filename = activeClient.getRequest().getFilename();
    std::cout << "FILENAME: " << filename << std::endl;
    const ServerConfig *config = activeClient.getConfig();

    // MUST RETURN AFTER ERROR! 
    // if we fail here, stop the entire function immediately.
    if (activeClient.getResponse().getStatusCode() != 200)
    {
        returnErrorPage(activeClient);
        return; 
    }

    switch (activeClient.getResponse().getStatusCode())
    {
        case 200:
            {
                // was using hete uri, when we should use key, this was previously returning null with the uri
                const RouteConfig *route = config->getRoute(locationKey); 
                
                // safely handle missing routes FIRST
                if (route == NULL)
                {
                    filepath = "var/www/html/index.html"; // Fallback
                    break;
                }

                //  validate methods
                if(!validateMethod(route, activeClient.getRequest().getMethod()))
                {
                    activeClient.getResponse().setStatusCode(405);
                    returnPage(activeClient);
                    activeClient.setState(FINISHED);
                    return;
                }

                //  NOW it is safe to touch the route's internals!
                std::string root = route->vectorRoute.at("root").at(0); 

                // determine what exactly they are asking for (directory or file?)
                if (filename.empty() || filename == "/")
                {
                    // they want the directory. safely check if an 'index' rule exists!
                    std::unordered_map<std::string, std::vector<std::string> >::const_iterator it = route->vectorRoute.find("index");
                    
                    if (it != route->vectorRoute.end()) {
                        filepath = root + "/" + it->second.at(0);
                    } else {
                        // no index found in config, might trigger 403 or autoindex later
                        filepath = root + "/"; 
                    }
                } 
                else 
                {
                    // they asked for a specific file (picture!)!
                    if (filename[0] == '/')
                        filepath = root + filename;
                    else
                        filepath = root + "/" + filename;
                }
                break;
            }

        default:
            filepath = "var/www/errorpages/default.html";
            break;
    }
    
    std::string response = createResponse(activeClient, filepath);

    // Warning: Direct write() is blocking. We will move this to POLLOUT later!
    write(activeClient.getFd(), response.c_str(), response.size());
}
