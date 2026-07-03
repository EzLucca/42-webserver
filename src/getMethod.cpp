#include "getMethod.hpp"
#include <filesystem>

std::string createResponse(Client& activeClient, std::string filepath)
{
    int code = activeClient.getResponse().getStatusCode();
    std::string statusCode = std::to_string(code);
    std::string statusmsg = activeClient.getResponse().getStatusMessage();
    std::string contentType = activeClient.getResponse().getMimeType(filepath);
    std::ifstream file(filepath.c_str(), std::ios::binary); // NEED TO OPEN IN BINARY MODE
    if (!file.is_open())
    {
        std::cerr << "Could not open: " << filepath << std::endl;
        activeClient.getResponse().setStatusCode(404);
        activeClient.getResponse().setStatusMessage("Not found");
        activeClient.setState(ERROR);
        return ("HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 48\r\n\r\n<html><body><h1>404 File Not Found</h1></body></html>");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string body = buffer.str();

    activeClient.getResponse().setResponseBody(body);
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
    activeClient.getResponse().setResponseBuffer(response);
    activeClient.setState(WRITING_RESPONSE);

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
std::string generateAutoindex(std::string filepath, std::string filename, std::string uriRequest)
{
    std::ostringstream html;

    // make sure it has trailing slash
    std::string baseUri = uriRequest;
    if (!baseUri.empty() && baseUri.back() != '/') {
        baseUri += "/";
    }

    html << "<html><head><title>Index of " << baseUri << "</title></head>";
    html << "<body><h1>Index of " << baseUri << "</h1><ul>";

    try
    {
        for (const auto &entry : std::filesystem::directory_iterator(filepath + "/" + filename))
        {
            std::string name = entry.path().filename().string();

            if (name == "." || name == "..")
                continue;

            std::string display_name = html_escape(name);
            std::string link = name;

            if (std::filesystem::is_directory(entry.path()))
            {
                display_name += "/";
                link += "/";
            }

            //add baseuri before link
            html << "<li><a href=\"" << baseUri  << link << "\">"
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

    const ServerConfig *config = activeClient.getConfig();
    std::string autoindexbody;

    if (activeClient.getResponse().getStatusCode() != 200)
    {
        returnErrorPage(activeClient);
        return; 
    }

    switch (activeClient.getResponse().getStatusCode())
    {
        case 200:
            {
                const RouteConfig *route = config->getRoute(locationKey); 

                if (route == NULL)
                {
                    filepath = "var/www/html/index.html"; // Fallback
                    break;
                }

                std::unordered_map<std::string, std::vector<std::string> >::const_iterator redirectIt =
                    route->vectorRoute.find("return");
                if (redirectIt != route->vectorRoute.end() &&
                        redirectIt->second.size() >= 2)
                {
                    int code = std::stoi(redirectIt->second[0]);
                    std::string target = redirectIt->second[1];

                    std::stringstream response;

                    response << "HTTP/1.1 "
                        << code
                        << " Moved Permanently\r\n";

                    response << "Location: "
                        << target
                        << "\r\n";

                    response << "Content-Length: 0\r\n";
                    response << "\r\n";

                    activeClient.getResponse().setResponseBuffer(response.str());
                    activeClient.setState(WRITING_RESPONSE);

                    return;
                }

                std::unordered_map<std::string, std::vector<std::string> >::const_iterator rootIt = route->vectorRoute.find("root");
                if (rootIt == route->vectorRoute.end() || rootIt->second.empty())
                {
                    activeClient.getResponse().setStatusCode(500);
                    activeClient.getResponse().setStatusMessage("Internal Server Error: Route configuration missing root");
                    activeClient.setState(ERROR);
                    return;
                }
                std::string root = rootIt->second.at(0);

                std::cout << activeClient.getRequest().getAutoindex() << std::endl;
                std::cout << root + "/" + filename << " " << std::filesystem::is_directory(root + "/" + filename) << std::endl;
                if (std::filesystem::is_directory(root + "/" + filename) || filename.empty() || filename == "/")
                {
                    // they want the directory. safely check if an 'index' rule exists!
                    std::unordered_map<std::string, std::vector<std::string> >::const_iterator it = route->vectorRoute.find("index");

                    std::cout << "inside: " << std::endl;
                    if (it != route->vectorRoute.end())
                    {
                        std::cout << "normal: " << std::endl;
                        filepath = root + "/" + it->second.at(0);
                    } 
                    else 
                    {
                        std::cout << "else: " << std::endl;
                        if (activeClient.getRequest().getAutoindex() == true)
                        {
                            std::cout << "root: " << root << std::endl;
                            std::cout << "uriRequest: " << uriRequest << std::endl;
                            autoindexbody = generateAutoindex(root, filename, uriRequest);
                            std::cout << "autoindex build" << std::endl;
                        }
                        else
                        {
                            activeClient.getResponse().setStatusCode(403);
                            activeClient.getResponse().setStatusMessage("Access denied");
                            activeClient.setState(ERROR);
                            return;
                        }
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

    if (!autoindexbody.empty())
    {
        std::string body = autoindexbody;

        std::string response = "HTTP/1.1 " + std::to_string(activeClient.getResponse().getStatusCode()) + " " + activeClient.getResponse().getStatusMessage() + "\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
        // response += "Connection: keep-alive\r\n\r\n";
        response += body;

        std::cout << "Response autoindex build" << std::endl;
        activeClient.getResponse().setResponseBuffer(response);
        activeClient.setState(WRITING_RESPONSE);

        return ;
    }

    //preparing for filestreaming
    activeClient.getResponse().prepareFileStream(filepath, activeClient);
    activeClient.setState(WRITING_RESPONSE);
}
