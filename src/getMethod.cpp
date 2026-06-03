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

void returnPage(Client& activeClient) 
{
    std::string filepath;
    std::string statusText;

    std::string uriRequest = activeClient.getRequest().getUri();
    std::cout << "Requested URI from inside return: " << uriRequest << std::endl; 
    statusText = activeClient.getResponse().getStatusMessage();

    // 2. Grab the direct pointer to the rulebook!
    const ServerConfig *config = activeClient.getConfig();

    // Safety check just in case HERE DO 
    /*
       TODO: Check allowed methods

       Path translation & hard drive check
       • translate the web uri into physical hard drive paath using
       the root directive of the matched location block. 
       CGI or static ?
       • AFter all previous things checked and ok, rules ok , file exists.
       ∘ Then you route to cgi ONLY IF BOTH ARE TRUE:  1.The matched location block has a cgi_pass directive configured.
       2.The physical file extension matches the CGI extension
       (e.g., it ends in .py or .php).

       If either of those is false. (e.g there is no cgi_pass, ot the file is .png, you route the request to the static master to just serve the file as standard binary/text\
       */
    std::cout << "Status Code: " << activeClient.getResponse().getStatusCode() << std::endl;
    switch (activeClient.getResponse().getStatusCode())
    {
        std::cout << "Status Code: " << activeClient.getResponse().getStatusCode() << std::endl;
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
                // std::cout << "throwing here" << std::endl;
                // throw std::invalid_argument("Method not allowed");
                activeClient.getResponse().setStatusCode(405);
                returnPage(activeClient);
                activeClient.setState(FINISHED);
                return;
            }

            if (route != NULL) {
                // TODO: parsing
                std::cout << "route is not null" << std::endl;
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

        case 400:
        // 3. Ask the config directly for the error page!
        filepath = config->getErrorPage(400);
        break;

        case 404:
        // 3. Ask the config directly for the error page!
        filepath = config->getErrorPage(404);
        break;

        case 405:
        // 3. Ask the config directly for the error page!
        filepath = config->getErrorPage(405);
        break;

        case 414:
        // 3. Ask the config directly for the error page!
        filepath = config->getErrorPage(414);
        break;

        case 500:
        filepath = config->getErrorPage(500); 
        // If the user didn't specify a 500 page in the .conf, use a hardcoded default
        if (filepath.empty()) 
            filepath = "var/www/errorpages/500.html";
        break;

        case 501:
        // 3. Ask the config directly for the error page!
        filepath = config->getErrorPage(501);
        break;

        case 505:
        // 3. Ask the config directly for the error page!
        filepath = config->getErrorPage(505);
        break;

        default:
        filepath = "var/www/errorpages/default.html";
        break;
    }

    std::ifstream file(filepath.c_str()); // .c_str() needed for C++98 ifstream

    if (!file.is_open())
    {
        std::cerr << "Could not open: " << filepath << std::endl;
        // In the future, this should change the status to 404/500 and recursively call returnPage
        activeClient.getResponse().setStatusCode(404);
        returnPage(activeClient);
        activeClient.setState(FINISHED);
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
