#!/usr/bin/env python3

import sys
import os

def main():

    print("Content-Type: text/html\r\n\r\n", end="")

    print("<html><body>")
    print("<h1>CGI Diagnostic Tool</h1>")
    # teste envp
    print("<h2>1. Environment Variables Received:</h2>")
    print("<ul>")
    for key, value in os.environ.items():
        print(f"<li><b>{key}</b>: {value}</li>")
    print("</ul>")

    print("<h2>2. Standard Input (Body) Received:</h2>")

    body_data = sys.stdin.read() 

    if body_data:
        print(f"<pre style='background:#eee; padding:10px;'>{body_data}</pre>")
    else:
        print("<p><i>No body data received from standard input.</i></p>")

    print("</body></html>")

if __name__ == "__main__":
    main()
