#!/usr/bin/env python3
import time
import sys

# 1. Immediately flush a debug print so you know it started (this goes to server logs, not the browser)
print("CGI Script started... going to sleep for 15 seconds.", file=sys.stderr)

# 2. The Freeze: We sleep longer than your C++ server's 10-second limit
time.sleep(15)

# 3. The Failure Condition: If the C++ server hasn't killed us by now, the timeout failed!
print("Content-Type: text/html\r\n\r\n", end="", flush=True)
print("<html><body><h1 style='color:red;'>TEST FAILED! The server did not kill the frozen CGI process!</h1></body></html>", flush=True)
