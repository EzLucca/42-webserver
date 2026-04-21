# Webserv --- 42 School Project

## Overview

**Webserv** is a custom HTTP server written in C++, developed as part of
the 42 School curriculum. The goal of this project is to understand how
web servers work by implementing one from scratch, without using
existing frameworks.

This server is inspired by real-world servers like Nginx and supports
handling multiple clients, HTTP parsing, and configurable behavior via a
configuration file.

------------------------------------------------------------------------

## Features

-   HTTP/1.1 compliant server
-   Non-blocking I/O using `poll()` (or `select()` / `epoll()` depending
    on implementation)
-   Configurable server using `.conf` file
-   Support for:
    -   GET
    -   POST
    -   DELETE
-   File serving (static websites)
-   Directory listing (autoindex)
-   CGI execution (e.g. Python, PHP)
-   Upload handling
-   Custom error pages
-   Multiple server blocks (virtual hosts)
-   Persistent connections (keep-alive)

------------------------------------------------------------------------

## What We Learned

-   Low-level networking (sockets, bind, listen, accept)
-   Event-driven architecture
-   HTTP protocol structure and parsing
-   Process management (for CGI)
-   File I/O and resource handling
-   Configuration parsing
-   Writing modular and maintainable C++ code

------------------------------------------------------------------------

## Project Structure

    webserv/
    ├── src/
    │   ├── server/
    │   ├── request/
    │   ├── response/
    │   ├── config/
    │   └── utils/
    ├── include/
    ├── config/
    │   └── default.conf
    ├── www/
    │   └── index.html
    ├── cgi-bin/
    ├── Makefile
    └── README.md

------------------------------------------------------------------------

## Getting Started

### 1. Clone the repository

``` bash
git clone https://github.com/yourusername/webserv.git
cd webserv
```

### 2. Compile

``` bash
make
```

### 3. Run the server

``` bash
./webserv config/default.conf
```

If no config is provided, a default configuration may be used.

------------------------------------------------------------------------

## Configuration File

The server uses a configuration file similar to Nginx:

``` conf
server {
    listen 8080;
    server_name localhost;

    root ./www;
    index index.html;

    location /images {
        root ./assets;
        autoindex on;
    }

    location /upload {
        method POST;
        upload_path ./uploads;
    }

    error_page 404 ./errors/404.html;
}
```

------------------------------------------------------------------------

## Supported HTTP Methods

  Method   Description
  -------- ---------------------------------
  GET      Retrieve a resource
  POST     Send data (e.g. forms, uploads)
  DELETE   Remove a resource

------------------------------------------------------------------------

## Testing

You can test the server using:

### Browser

    http://localhost:8080

### cURL

``` bash
curl -X GET http://localhost:8080
curl -X POST -d "data=test" http://localhost:8080/upload
curl -X DELETE http://localhost:8080/file.txt
```

------------------------------------------------------------------------

## CGI Support

The server executes CGI scripts based on file extension:

Example:

``` bash
/cgi-bin/script.py
```

Supports: - Python - PHP (if configured)

------------------------------------------------------------------------

## Limitations

-   Not production-ready
-   Basic HTTP compliance (not full RFC implementation)
-   Limited security features
-   Performance depends on implementation choices

------------------------------------------------------------------------

## Allowed Functions

-   `socket`, `bind`, `listen`, `accept`
-   `recv`, `send`
-   `poll` / `select` / `epoll`
-   `fork`, `execve`, `pipe`
-   Standard C++98 libraries only

------------------------------------------------------------------------
