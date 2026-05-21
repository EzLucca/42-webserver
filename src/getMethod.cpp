#include "getMethod.hpp"

void returnPage(Client activeClient, ServerManager server)
{
    std::string filepath;
    std::string statusText;

    // TODO: check where the header referer value is stored and validade.
    std::string uriRequest = activeClient.getRequest().getUri();
    std::cout << uriRequest << std::endl; 

    switch (activeClient.getResponse().getStatusCode())
    {
        // TODO:change for the config values
        case 200:
            filepath = server.getServerLocation("mysite.com", "/", "root")
            + '/' + server.getServerLocation("mysite.com", "/", "index");
            statusText = "200 OK";
            break;

        case 404:
            filepath = server.getServerErrorPages("mysite.com",404);
            statusText = "404 Not Found";
            break;

        case 500:
            filepath = "var/www/errorpages/500.html";
            statusText = "500 Internal Server Error";
            break;

        default:
            filepath = "var/www/errorpages/default.html";
            statusText = "400 Bad Request";
            break;
    }

    std::ifstream file(filepath);

    if (!file.is_open())
    {
        std::cerr << "Could not open: " << filepath << std::endl;
        // TODO: throw error
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string body = buffer.str();

    std::string response =
        "HTTP/1.1 " + statusText + "\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" +
        body;

    write(activeClient.getFd(), response.c_str(), response.size());
}
