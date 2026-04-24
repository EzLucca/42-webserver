#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

#include <iostream>

//HttpRequest consist of request line, headers (body and query optional)
// Class holds the parsed URI, Method and headers
class HttpRequest
{
    // A request line consists of method, request target and HTTP version (ALL MANDATORY)
    //For example: GET /api/users HTTP/1.1

    // Methods need to be implemented GET, POST or DELETE

    //HEADERS. Common request headers include:
    // Host, Content-Type, Authorization, User-Agent, Accept, Connection, Accept-Language


    //BODY contains data sent to the server

    //QUERY PARAMETERS, are key-value pairs appended to the url to filter or sort data



};

#endif