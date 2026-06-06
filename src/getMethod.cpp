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
    std::ifstream file(filepath.c_str()); // .c_str() needed for C++98 ifstream

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
        "Content-Type: text/html\r\n"
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
    std::string statusText;

    std::string uriRequest = activeClient.getRequest().getUri();
    std::cout << "Requested URI from inside return: " << uriRequest << std::endl; 
    // statusText = activeClient.getResponse().getStatusMessage();
    statusText = activeClient.getResponse().getStatusCode();

    // 2. Grab the direct pointer to the rulebook!
    const ServerConfig *config = activeClient.getConfig();

    std::cout << "Status Code: " << activeClient.getResponse().getStatusCode() << std::endl;
    if (activeClient.getResponse().getStatusCode() != 200)
        returnErrorPage(activeClient);
    switch (activeClient.getResponse().getStatusCode())
    {
        case 200:
            {
                const RouteConfig *route = config->getRoute(uriRequest); 

                // TODO: Check location
                // TODO: Check if autoindex is on
                // TODO: Method validation 
                if (route == NULL)
                    break;
                if(!validateMethod(route, activeClient.getRequest().getMethod()))
                {
                    activeClient.getResponse().setStatusCode(405);
                    returnPage(activeClient);
                    activeClient.setState(FINISHED);
                    return;
                }

                if (route != NULL) {
                    // TODO: parsing
                    std::cout << "route is not null" << std::endl;
                    // TODO PROBLEM WITH THIS APPROACH. 
                    std::string root = route->vectorRoute.at("root").at(0);
                    std::string index = route->vectorRoute.at("index").at(0);
                    filepath = root + "/" + index;
                    // filepath = root + "/";
                } else {
                    std::cout << "route is null" << std::endl;
                    filepath = "var/www/html/index.html"; // Fallback if route not found
                }
                break;
            }

        default:
            filepath = "var/www/errorpages/default.html";
            break;
    }
    std::string response;
    response = createResponse(activeClient, filepath);

    // Warning: Direct write() is blocking. We will move this to POLLOUT later!
    write(activeClient.getFd(), response.c_str(), response.size());
}
