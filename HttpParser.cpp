#include <HttpParser.hpp>

void HttpParser::parseRequestLine(std::string& line, HttpRequest& request)
{
    //we got the line with all of the information
    size_t firstSpace = line.find(' ');
    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (firstSpace != std::string::npos && secondSpace != std::string::npos)
    {
        // now we parse
        request.setMethod(line.substr(0, firstSpace)); // DOUBLE CHECK
        request.setUri(line.substr(firstSpace + 1, secondSpace - firstSpace - 1)); // CHECK THAT THIS WORKS
        request.setVersion(line.substr(secondSpace +1)); // DOUBLE CHECK THIS TOO

        //TODO! VALIDATE METHOD URI AND VERSION BEFORE GOING FORWARD!
    }
    else 
    {
        //ERROR HANDLING HERE !!! if not all variables found, also we need to validate
    }
}

void HttpParser::parseAllHeaders(std::string& rawHeaders, HttpRequest& request)
{
    // main logic here is that we parse line by line calli singleheader parse function, and after the parse, we erase that part from rawHeaders
    while (rawHeaders.length() > 0)
    {   
        size_t singleHeaderLength = rawHeaders.find("\r\n");
        if (singleHeaderLength == std::string::npos)
        {
            //abort mission
            break;
        }
        if (singleHeaderLength == 0) //if we hit the \r\n in the index 0, we know we are in the end of headers so \r\n\r\n
        {
            rawHeaders.erase(0,2); // delete last \r\n
            break;
        }
        std::string singleHeader = rawHeaders.substr(0, singleHeaderLength);
        parseSingleHeader(singleHeader, request); //lets parse it
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


void HttpParser::parseSingleHeader(std::string& line, HttpRequest& request)
{
    size_t colonPos = line.find(':');

    if (colonPos != std::string::npos)
    {
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        //HTTP standard has OWS ( optional whitespace), so after parsing value we need to check if there is space before the value!
        value = trimSpaces(value);
        // do we want to lowercase the key and value ???
        //if key == Content-length, we need to check the value is valid, (maybe with stoi in a try catch? )
        request.setHeader(key, value); // add to the headers.

    }
    else if (colonPos == std::string::npos)
    {
        // ERROR HANDLING, if we cant find :, that means its a bad request
    }
}

void HttpParser::parseChunkedBody(std::string& rawBody, HttpRequest& request)
{

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
        else
        {
            // we land here if we didnt find the word, so if for example have small buffersize, and have a partial requestline.
            // here we just return and do nothing.
            return ;
        }
        //if parsing is done change the state!
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
            std::string line = workBuffer.substr(0, pos);

            parseAllHeaders(line, client.getRequest());

            // erase headers from the buffer and change state!
            client.eraseFromBuffer(pos + 4); // +4 because we are infront of \r\n\r\n

            // WE ARE SETTING THE STATE OF READIING BODY ONLY if we have headers like "Content length" and or "Transfer-Encoding chunked."
            client.setState(READING_BODY); // set state to the next thing, so reading headers.
        }
        return ;
    }
    // 3. we parse the body, here we are comparing the content length number to the actual size of the vector. when the size == to the content length, we know thats end of the body
        //we just append all the bytes until we have appended the same amount the parsed contentlength value is. 
    if (client.getState() == READING_BODY) //OPTIONAL HOW WE CHECK THIS? 
    {

        //if parsing is done change the state!
    }

    // REMEMBER TO CHANGE THE STATE in EVERYSTEP
}

