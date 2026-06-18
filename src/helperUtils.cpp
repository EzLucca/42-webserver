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
void validatePath(const std::string &filePath)
{
    if (!std::filesystem::exists(filePath))
        throw std::invalid_argument("path does not exist: " + filePath);
}

bool    validateUriPath(Client &activeClient)
{
    std::string uriPathRequest = activeClient.getRequest().getUriPath();
    std::vector<std::string> locationlist =
        activeClient.getConfig()->getLocationList();
    bool found = false;

    for (const std::string &location : locationlist) {
        if (location == uriPathRequest) {
            found = true;
            break;
        }
    }
    if (found)
    {
        activeClient.getResponse().setStatusCode(200);
        activeClient.getResponse().setStatusMessage("OK");
        return (true);
    }
    activeClient.getResponse().setStatusCode(404);
    activeClient.getResponse().setStatusMessage("Not found");
    activeClient.setState(ERROR); 
    return (false);
}

size_t getBodyClient(Client &activeClient)
{
    size_t bodyClientMax = activeClient.getConfig()->getClientMaxBodySize();

    const ServerConfig *config = activeClient.getConfig();
    const RouteConfig *route =
        config->getRoute(activeClient.getRequest().getLocationKey());

    std::unordered_map<std::string, std::vector<std::string>>::const_iterator it =
        route->vectorRoute.find("client_max_body_size");

    if (it != route->vectorRoute.end() && !it->second.empty()) {
        std::string val = it->second[0];
        if (val.back() == 'M') {
            int size = std::stoi(val.substr(0, val.size() - 1));
            bodyClientMax = size * 1024 * 1024;
            return (bodyClientMax);
        }
    }
    return (bodyClientMax);
}

std::string buildSafeTargetPath(const RouteConfig* route, const std::string& uri) 
{
    std::string root = "";

    //  Safely extract "root" without using the explosive .at()
    if (route != NULL) 
    {
        std::unordered_map<std::string, std::vector<std::string>>::const_iterator it = route->vectorRoute.find("root");
        if (it != route->vectorRoute.end() && !it->second.empty()) 
        {
            root = it->second[0];
        }
    }

    // Fallback if your friend's config block didn't have a root
    if (root.empty()) 
    {
        root = "var/www/html"; // Change this to your project's default directory
    }

    // Glue them together
    std::string fullPath = root + "/" + uri;

    size_t pos = 0;
    while ((pos = fullPath.find("//", pos)) != std::string::npos) 
    {
        fullPath.replace(pos, 2, "/");
    }

    return fullPath;
}

bool endsWith(const std::string& fullString, const std::string& ending) 
{
    if (fullString.length() >= ending.length()) 
    {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }
    return false;
}

