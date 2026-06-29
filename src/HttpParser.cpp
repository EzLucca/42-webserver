#include "HttpParser.hpp"
#include "HttpException.hpp"
#include "helperUtils.hpp"

#define MAX_REQUEST_LINE 8192
#define MAX_HEADER_BYTES 65536 

void HttpParser::parseRequestLine(std::string& line, HttpRequest& request)
{
    //we got the line with all of the information
    size_t firstSpace = line.find(' ');
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (firstSpace != std::string::npos && secondSpace != std::string::npos)
    {
        request.setMethod(line.substr(0, firstSpace));
        request.setUri(line.substr(firstSpace + 1, secondSpace - firstSpace - 1));
        request.setVersion(line.substr(secondSpace +1));

        //validating methods
        if(request.getMethod() != "GET" && request.getMethod() != "POST" && request.getMethod() != "DELETE")
        {
            throw HttpException(501, "Not implemented.");
        }
        //validating uri, first check length 
        size_t uriLen = request.getUri().size();
        std::string uri = request.getUri();
        if (uriLen > 2048)
        {
            throw HttpException(414, "URI Too Long");
        }
        if (request.getUri().empty() || request.getUri().at(0) != '/') //Check if works
        {
            throw HttpException(400, "Bad Request: URI must start with '/'");
        }
        //validate uri characteres
        for (size_t i = 0; i < uriLen; ++i) 
        {
            char c = uri[i];
            // check for spaces and not printable characters
            if (c <= 32 || c >= 127) { 
                throw HttpException(400, "Bad Request: Invalid character in URI");
            }
        }

        //validating version
        if (request.getVersion() != "HTTP/1.1")
        {
            throw HttpException(505, "HTTP version not supported.");
        }
		std::string	lowerUri = stringToLower(uri);
		if (lowerUri == "/.." || lowerUri.find("\\") != std::string::npos || lowerUri.find("/../") != std::string::npos
			|| (lowerUri.size() >= 3 && lowerUri.compare(lowerUri.size() - 3, 3, "/..") == 0)
			|| lowerUri.find("%2e%2e") != std::string::npos || lowerUri.find("%2f") != std::string::npos
			|| lowerUri.find("%5c") != std::string::npos)
			throw HttpException(403, "Forbidden");
        //DEBUGGING!!
        std::cout << "Parsed method: " << request.getMethod() << "\n"
            << "Parsed Uri :" << request.getUri() << "\n"
            << "Parsed version : " << request.getVersion() << std::endl;
    }
    else 
    {
        //If the whole request line is not parsed, we return to wait more data
        return ;
    }
}


void HttpParser::parseAllHeaders(std::string& rawHeaders, Client& client)
{
    // main logic here is that we parse line by line calli singleheader parse function, and after the parse, we erase that part from rawHeaders
    while (rawHeaders.length() > 0)
    {   
        size_t singleHeaderLength = rawHeaders.find("\r\n");
        if (singleHeaderLength == std::string::npos)
        {
            // return to wait more data
            break;
        }
        if (singleHeaderLength == 0) // if we hit the \r\n in the index 0, we know we are in the end of headers so \r\n\r\n
        {
            rawHeaders.erase(0,2); // delete last \r\n
            break;
        }
        std::string singleHeader = rawHeaders.substr(0, singleHeaderLength);
        // parseSingleHeader(singleHeader, request); //lets parse it
        parseSingleHeader(singleHeader, client); //lets parse it
        rawHeaders.erase(0, singleHeaderLength + 2); //erase the parsed header 
    }
}

std::string HttpParser::trimSpaces(std::string& value)
{
    //we can have 0-2 spaces before our value, according to http protocol
    size_t startPos = value.find_first_not_of(' ');
    if (startPos == std::string::npos) //if all values are whitespaces
        return ("");
    size_t endPos = value.find_last_not_of(' ');
    return (value.substr(startPos, endPos - startPos + 1));
}

std::string HttpParser::stringToLower(std::string value)
{
    std::string s = value;
    for (size_t i = 0; i < s.length(); ++i) 
    {
        s[i] = std::tolower(s[i]);
    }
    return (s);
}

void HttpParser::parseSingleHeader(std::string& line, Client& client)
{
    size_t colonPos = line.find(':');

    if (colonPos != std::string::npos)
    {
        std::string key = line.substr(0, colonPos); 
        std::string value = line.substr(colonPos + 1);
        std::string combinedValue;
        std::map<std::string, std::string> currentHeaders = client.getRequest().getHeaders();
        //HTTP standard has OWS ( optional whitespace), so after parsing value we need to check if there is space before the value!
        value = trimSpaces(value);
        //we need to lowercase ALL the headerkeys, because they are case insensitive in http1.0
        key = stringToLower(key);

        if (currentHeaders.find(key) != currentHeaders.end()) 
        {
            if (key == "host" || key == "content-length") 
            {
                throw HttpException(400, "Bad Request: Duplicate critical header");
            }
            combinedValue = currentHeaders[key] + ", " + value;
            client.getRequest().setHeader(key, combinedValue);
        } 
        else 
        {
          client.getRequest().setHeader(key, value);
        }
        if (key == "connection"  && value == "close")
        {
                client.getRequest().setKeepAlive(false);
        }
        if (key == "content-length")
        {   
            try
            {
                // TEST:

                // ValidateBodyLength with the config on the client;
                size_t bodyClientMax = getBodyClient(client);

                client.getRequest().setContentLength(value);
                // Maybe we should try catch this, need somekind of validation check
                if (client.getRequest().getContentLength() > bodyClientMax)
                    throw HttpException(400, "Bad Request: Content-Length is astronomically large");
            }
            catch (const std::invalid_argument& e) {
                // catching characaters
                throw HttpException(400, "Bad Request: Invalid Content-Length format");
            } 
            catch (const std::out_of_range& e) {
                // catching too big numbers 
                throw HttpException(400, "Bad Request: Content-Length is astronomically large");
            }
        }
        if (key == "transfer-encoding" && value == "chunked")
        {
            client.getRequest().setIsChunked();
            std::cout << "Chunked is flagged " << client.getRequest().getIsChunked() << std::endl; //DEBUGGING
        }
        // request.setHeader(key, value); // add to the headers.
        client.getRequest().setHeader(key, value); // add to the headers.

    }
    else if (colonPos == std::string::npos)
    {
        // ERROR HANDLING, if we cant find :, that means its a bad request
        throw HttpException(400, "Bad Request: Malformed header (missing colon)");
    }
}


void HttpParser::validateHeaders(HttpRequest& request)
{
    //we need to validate 3 things.  host is mandatory, so if we have multiple sites, we know which config block to use
    const std::map<std::string, std::string>& headers = request.getHeaders();
    //check for host from the map
    //try to find the key host
    std::map<std::string, std::string>::const_iterator it = headers.find("host");
    if (it == headers.end())
    {
        //couldnt find a key, whole map searched through
        throw HttpException(400, "Bad Request: Missing host header.");
    }

    bool hasContentHeader = false;
    bool hasTransferEncodingHeader = false;

    it = headers.find("content-length");
    if (it != headers.end())
        hasContentHeader = true;
    it = headers.find("transfer-encoding");
    if (it != headers.end())
        hasTransferEncodingHeader = true;

    //check transfer encoding value, if its something else than chunked, for example gzip. return 501 Not implemented: Unsupported transfer-encoding
    if (hasTransferEncodingHeader == true)
    {
        if (it->second != "chunked")
            throw HttpException(501, "Not implemented.");
    }
    if (hasContentHeader && hasTransferEncodingHeader)
        throw HttpException(400, "Bad Request: Content-Length and Transfer-Encoding conflict (Smuggling attempt detected)");

}


void HttpParser::parseBodyIntoFile(int clientFd, std::string& bodyData, HttpRequest& request)
{
	bool firstWrite = request.getBodyFilePath() == "not-set";
	if (firstWrite)
		request.setBodyFilePath("temp_body_" + std::to_string(clientFd) + ".bin");
	std::ofstream outFile(request.getBodyFilePath().c_str(), std::ios::out | std::ios::binary 
	| (firstWrite ? std::ios::trunc : std::ios::app));
    if (!outFile.is_open())
        throw HttpException(500, "Internal Server Error: Could not open temp file for writing");

    outFile.write(bodyData.data(), bodyData.size());

    outFile.close();
}


HttpParser::HttpParser()
{
    std::cout << "HttParser constructor called." << std::endl;
}

HttpParser::~HttpParser()
{
    std::cout << "HttParser destructor called." << std::endl;
}

void HttpParser::parse(Client& client)
{
    if (client.getState() == READING_REQUESTLINE)
    {
        const std::string& workBuffer = client.getBuffer();
        size_t pos = workBuffer.find("\r\n");
        if (workBuffer.size() > MAX_REQUEST_LINE)
            throw HttpException(400, "Malformed request line");
        if (pos != std::string::npos)
        {
            std::string line = workBuffer.substr(0, pos);
            parseRequestLine(line, client.getRequest());
            client.getRequest().setupPathKeys(client);
            client.getRequest().validateAutoindex(client);
            client.eraseFromBuffer(pos + 2);
            client.setState(READING_HEADERS);

        }
    }

    if (client.getState() == READING_HEADERS)
    {
        const std::string& workBuffer = client.getBuffer();
        size_t pos = workBuffer.find("\r\n\r\n");
        if (workBuffer.size() > MAX_HEADER_BYTES)
            throw HttpException(400, "Malformed header");
        if (pos != std::string::npos)
        {
            std::string line = workBuffer.substr(0, pos + 2);

            parseAllHeaders(line, client);

            client.eraseFromBuffer(pos + 4);

            HttpRequest request = client.getRequest();
            request.printHeaders();

            if (request.getIsChunked())
            {

                client.setState(READING_BODY_CHUNKED);
            }
            else if (request.getContentLength() > 0)
            {
                client.setState(READING_BODY);
            }
            else
            {
                client.setState(PROCESSING);
            }
        }

    }
    while (client.getState() == READING_BODY_CHUNKED)
    {
        const std::string& workBuffer = client.getBuffer();

        long chunkSize = client.getRequest().getCurrentChunkSize();

        if (chunkSize == -1)
        {
            size_t pos = workBuffer.find("\r\n");
            if (pos != std::string::npos)
            {
                std::string line = workBuffer.substr(0, pos);
                client.getRequest().setCurrentChunkSize(line);
                client.eraseFromBuffer(pos + 2);
            }
            else
            {
                return ;
            }
        }
        else
        {
            if (chunkSize < 0) // guard before casting to size_t
                throw HttpException(400, "Bad Request: Invalid chunk size");
            if (chunkSize == 0)
            {
                client.setState(PROCESSING);
                client.eraseFromBuffer(2);
                return ;
            }
            if (workBuffer.size() <= ((size_t)chunkSize + 1)) // EXPERIMENTAL
            {
                return ;
            }

            if (workBuffer.size() >= ((size_t)chunkSize + 2))
            {
                std::string line = workBuffer.substr(0, chunkSize);
                parseBodyIntoFile(client.getFd(), line, client.getRequest());
                client.getRequest().setFullChunkBodySize(chunkSize);
                size_t bodyClientMax = getBodyClient(client);
                if (client.getRequest().getFullChunkBodySize() > static_cast<long>(bodyClientMax))
                    throw HttpException(400, "Bad Request: Content-Length is astronomically large");
                client.eraseFromBuffer(chunkSize + 2);
                client.getRequest().resetCurrentChunkSize();
            }

        }

    }

    if (client.getState() == READING_BODY)
    {
        const std::string& bodyBuffer = client.getBuffer();

        // fd 
        if (!bodyBuffer.empty()) {
            size_t bytesToWrite = bodyBuffer.size();
            size_t bytesRemaining = client.getRequest().getContentLength() - client.getRequest().getBytesWritten();

            // make sure we dont accidentally write from new request
            if (bytesToWrite > bytesRemaining) {
                bytesToWrite = bytesRemaining; 
            }

            std::string chunkToWrite = bodyBuffer.substr(0, bytesToWrite);
            parseBodyIntoFile(client.getFd(), chunkToWrite, client.getRequest());

            // update state, free ram
            client.getRequest().addBytesWritten(bytesToWrite);
            client.eraseFromBuffer(bytesToWrite);
        }

        // Check if ready
        if (client.getRequest().getBytesWritten() >= client.getRequest().getContentLength()) 
        {
            client.setState(PROCESSING);
        }

    }
    else 
    {
        return ;
    }

}



