#include "helperUtils.hpp"

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

