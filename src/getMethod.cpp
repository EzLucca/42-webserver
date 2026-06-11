#include "getMethod.hpp"
#include <filesystem>

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
    std::string response;

    std::string uriRequest = activeClient.getRequest().getUri();
    std::cout << "URI from inside returnErrorPage: " << uriRequest << std::endl; 
    std::cout << "ERROR STATUS CODE: " << activeClient.getResponse().getStatusCode() << std::endl;

    const ServerConfig *config = activeClient.getConfig();
    int code = activeClient.getResponse().getStatusCode();

    filepath = config->getErrorPage(code);

    response = createResponse(activeClient, filepath);

    // Warning: Direct write() is blocking. We will move this to POLLOUT later!
    write(activeClient.getFd(), response.c_str(), response.size());
}

static std::string html_escape(const std::string &s)
{
    std::string out;
    out.reserve(s.size());

    for (char c : s)
    {
        switch (c)
        {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
        }
    }
    return out;
}

std::string generateAutoindex(std::string filepath, std::string uriRequest)
{
    std::ostringstream html;

    html << "<html><head><title>Index of " << uriRequest << "</title></head>";
    html << "<body><h1>Index of " << uriRequest << "</h1><ul>";

    try
    {
        for (const auto &entry : std::filesystem::directory_iterator(filepath))
        {
            std::string name = entry.path().filename().string();

            // Optional: skip hidden files
            if (name == "." || name == "..")
                continue;

            std::string display_name = html_escape(name);
            std::string link = uriRequest;

            if (link.back() != '/')
                link += '/';

            link += name;

            if (std::filesystem::is_directory(entry.path()))
                display_name += "/";

            html << "<li><a href=\"" << link << "\">"
                << display_name << "</a></li>";
        }
    }
    catch (const std::exception &e)
    {
        return "<html><body><h1>403 Forbidden</h1></body></html>";
    }

    html << "</ul></body></html>";

    return html.str();
}

void    returnPage(Client& activeClient) 
{
    std::string filepath;
    std::string locationKey = activeClient.getRequest().getLocationKey();
    std::string uriRequest = activeClient.getRequest().getUri();
    std::string filename = activeClient.getRequest().getFilename();
    std::cout << "FILENAME: " << filename << std::endl;
    const ServerConfig *config = activeClient.getConfig();
    std::string autoindexbody;
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

                if (filename.empty() || filename == "/")
                {
                    // they want the directory. safely check if an 'index' rule exists!
                    std::unordered_map<std::string, std::vector<std::string> >::const_iterator it = route->vectorRoute.find("index");

                    if (it != route->vectorRoute.end()) {
                        filepath = root + "/" + it->second.at(0);
                    } else 
                        autoindexbody = generateAutoindex(root, uriRequest);
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
    std::string response;

    if (!autoindexbody.empty())
    {
        std::string body = autoindexbody;

        response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "\r\n" +
            body;
    }
    else
        response = createResponse(activeClient, filepath);

    // Warning: Direct write() is blocking. We will move this to POLLOUT later!
    write(activeClient.getFd(), response.c_str(), response.size());
}
