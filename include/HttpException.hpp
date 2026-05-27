# pragma once

#include <exception>
#include <string>


class HttpException : public std::exception 
{

private:
    int _statusCode;
    std::string _message;

public:
    
    HttpException(int code, const std::string& msg) : _statusCode(code), _message(msg) {}
    
    virtual ~HttpException() throw() {}
    
    virtual const char* what() const throw() { return _message.c_str(); }
    
    int getStatusCode() const { return _statusCode; }
    std::string getStatusMessage() const { return _message; }
};
