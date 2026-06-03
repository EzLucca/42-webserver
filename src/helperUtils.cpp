#include "helperUtils.hpp"
#include "Client.hpp"

/**
 * @brief Checks if a file or directory exists.
 *
 * This function uses std::filesystem to verify whether the given file path
 * exists. If the path is invalid, it prints an error message and terminates
 * the program.
 *
 * @param filePath Path to the file or directory to check.
 */
void validatePath(const std::string& filePath)
{
    if (!std::filesystem::exists(filePath))
        throw std::invalid_argument("path does not exist: " + filePath);
}

void    validateUriPath(Client &activeClient)
{
    std::string uriPathRequest = activeClient.getRequest().getUriPath();
    std::vector<std::string> locationlist = activeClient.getConfig()->getLocationList();
    bool found = false;

    for (const std::string& location : locationlist)
    {
        if (location == uriPathRequest)
        {
            found = true;
            break;
        }
    }
    if (found)
        activeClient.getResponse().setStatusCode(200);
    else
        activeClient.getResponse().setStatusCode(404);
}

// void    updateBodySize(Client &activeClient)
// {
//
//     const ServerConfig *config = activeClient.getConfig();
//     const RouteConfig *route = config->getRoute(activeClient.getRequest().getLocationKey());
//
//     std::unordered_map<std::string, std::vector<std::string>>::const_iterator it = route->vectorRoute.find("client_max_body_size");
//
//     if (it != route->vectorRoute.end() && filename.find(".py") != std::string::npos) 
//     {
//
//     }
// }
