#include "HttpParser.hpp"
#include "HttpException.hpp"

void HttpParser::parseRequestLine(std::string& line, HttpRequest& request)
{
    //we got the line with all of the information
    size_t firstSpace = line.find(' ');
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (firstSpace != std::string::npos && secondSpace != std::string::npos)
    {
        // now we parse
        request.setMethod(line.substr(0, firstSpace));
        //Validate method, if not valid, return
        request.setUri(line.substr(firstSpace + 1, secondSpace - firstSpace - 1));
        //validate Uri, if not valid return
        request.setVersion(line.substr(secondSpace +1));
        //validate version, if not valid return

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

// void HttpParser::parseAllHeaders(std::string& rawHeaders, HttpRequest& request)
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

// void HttpParser::parseSingleHeader(std::string& line, HttpRequest& request)
void HttpParser::parseSingleHeader(std::string& line, Client& client)
{
    size_t colonPos = line.find(':');

    if (colonPos != std::string::npos)
    {
        std::string key = line.substr(0, colonPos); 
        std::string value = line.substr(colonPos + 1);
        //HTTP standard has OWS ( optional whitespace), so after parsing value we need to check if there is space before the value!
        value = trimSpaces(value);
        //we need to lowercase ALL the headerkeys, because they are case insensitive in http1.0
        key = stringToLower(key); // DOESNT WORK, FIX!

        if (key == "connection")
        {
            if (value == "closed")
                // request.setKeepAlive(false);
                client.getRequest().setKeepAlive(false);
        }
        if (key == "content-length")
        {   
            try
            {
                // extract the key and value (value into number) and save to request object
                // request.setContentLength(value); // Maybe we should try catch this, need somekind of validation check
                // TODO: validateBodyLength with the config on the client;
                size_t bodyClientMax = client.getConfig()->getClientMaxBodySize();
                int parsedLength = std::stoi(value);

                if (parsedLength < 0) {
                    throw std::invalid_argument("Negative Content-Length"); 
                }
                if (static_cast<size_t>(parsedLength) > bodyClientMax)
                    throw HttpException(400, "Bad Request: Content-Length is astronomically large");
                client.getRequest().setContentLength(value); // Maybe we should try catch this, need somekind of validation check
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
            // request.setIsChunked();
            // std::cout << "Chunked is flagged " << request.getIsChunked() << std::endl; //DEBUGGING

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
    //ios::binary flag to make sure the data is written in raw binary and not touched
    //ios::app (append) flag to make sure the pointer is in the end of the file always when we write. 

    //fstream has an internal buffer of ~4Kb (using RAM), so when we are writing into a file, its actually written after 4kb, or manual flush call
    //filenaming needs to be unique, lets have client fd for example added there.
    // in case of keep alive connection, check if there is already temp file from previous request. if there is, remove the old before creating new

    request.setBodyFilePath("temp_body_" + std::to_string(clientFd) + ".bin");
    std::ofstream outFile(request.getBodyFilePath(), std::ios::out | std::ios::app | std::ios::binary);
    if (!outFile.is_open())
        throw HttpException(500, "Internal Server Error: Could not open temp file for writing");

    outFile.write(bodyData.data(), bodyData.size()); //.data of stringobject return pointer to raw , direct memory address where the actual bytes are stored. (that is why we need size, no null terminator there)

    //also remember the filepath 
    //we better to open and close file between writings. (this will also flush the fstream internal buffer), and this will protect us from fd limits.
    //we need to use the outFile.write(datachunkData, datachunkSize) to write safely in to the file. 
    //we need remember to increment the fullBodySize variable

    //filestreams automatically close when they get out of scope. but we should do it manually here just for safety
    outFile.close();

    //think about how to remove temp files, if connection drops out in the middle of reading body

}


HttpParser::HttpParser() // MAKE INITIALIZATION LIST
{
    std::cout << "HttParser constructor called." << std::endl;
}

HttpParser::~HttpParser()
{
    std::cout << "HttParser destructor called." << std::endl;
}

void HttpParser::parse(Client& client)
{
    //main logic in the parse

    //we land here after we have read the whatever client was sending
    // we have a order which we need to follow:
    // 1. we parse the request line, we are searching for \r\n and then we know its end of the request line
    //From here we get method, URI, and http version to our http request object
    if (client.getState() == READING_REQUESTLINE)
    {
        const std::string& workBuffer = client.getBuffer(); //lets have a reference, for optimization reasons
        size_t pos = workBuffer.find("\r\n");
        if (pos != std::string::npos)
        {
            //we found the \r\n, so our request line is fully in received.
            std::string line = workBuffer.substr(0, pos);
            parseRequestLine(line, client.getRequest());
            // then we need to erase the requestline part from the client buffer and change state
            client.eraseFromBuffer(pos + 2); // +2 because we are infront of \r\n
            client.setState(READING_HEADERS); // set state to the next thing, so reading headers.

        }
    }

    // 2. we parse the headers, now we are searching for \r\n\r\n to know we have read the headers.
    //From here we parse all the headers, to our map
    if (client.getState() == READING_HEADERS)
    {
        const std::string& workBuffer = client.getBuffer();
        size_t pos = workBuffer.find("\r\n\r\n"); //checking for the end of the headers 
        if (pos != std::string::npos)
        {
            //we found the \r\n\r\n, so our hearders are fully in received.
            std::string line = workBuffer.substr(0, pos + 2);

            // parseAllHeaders(line, client.getRequest());
            parseAllHeaders(line, client);

            // erase headers from the buffer and change state!
            client.eraseFromBuffer(pos + 4); // +4 because we are infront of \r\n\r\n

            // WE ARE SETTING THE STATE OF READIING BODY ONLY if we have headers like "Content length" and or "Transfer-Encoding chunked."
            HttpRequest request = client.getRequest();
            request.printHeaders();
            std::cout << "contentlength: " << request.getContentLength() << std::endl;

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
    // 3. we parse the body, here we are comparing the content length number to the actual size of the string. when the size == to the content length, we know thats end of the body
    //we just append all the bytes until we have appended the same amount the parsed contentlength value is. 
    while (client.getState() == READING_BODY_CHUNKED)
    {

        //std::cout << client.getRequest().getCurrentChunkSize() << std::endl;
        //in chunking we have phases Reading the size, and readint the data
        //our chunksize is initialized to -1, thats how we know we must read so:
        //PHASE 1
        const std::string& workBuffer = client.getBuffer();

        long chunkSize = client.getRequest().getCurrentChunkSize();

        if (chunkSize == -1)
        {
            size_t pos = workBuffer.find("\r\n"); //find the first chunksize value
            if (pos != std::string::npos)
            {
                std::string line = workBuffer.substr(0, pos); // now we have the hex value as string
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
            //Then we have the PHASE 2 where we read the actual data
            //This is when we know we are in the end of the body
            if (chunkSize == 0)
            {
                client.setState(PROCESSING);
                //client.getRequest().printBody();
                client.eraseFromBuffer(2); // we remove the last \r\n
                return ;
            }
            if (workBuffer.size() <= ((size_t)chunkSize + 1)) // EXPERIMENTAL
            {
                return ;
            }

            // otherwise we read the chunksize amount of data, remove it from the buffer, and then return our flag back to -1
            //we need to also check ofc that there is enough data in the buffer to read.
            if (workBuffer.size() >= ((size_t)chunkSize + 2)) //+2 because of the hanging \r\n
            {
                std::string line = workBuffer.substr(0, chunkSize);
                //client.getRequest().appendToBody(line); //This needs to be saved inside a file(not inside a string object)
                parseBodyIntoFile(client.getFd(), line, client.getRequest()); // this is now writing the chunk of data into a file.
                client.getRequest().setFullChunkBodySize(chunkSize); //increment full chunkbodysize after extracted the data,
                client.eraseFromBuffer(chunkSize + 2); // Free the buffer so we dont run into RAM problems.
                client.getRequest().setCurrentChunkSize("-0x1");
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



