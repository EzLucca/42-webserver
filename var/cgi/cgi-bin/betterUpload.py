#!/usr/bin/env python3
import sys
import os
import email
from email.parser import BytesParser

def main():
    # Print headers immediately
    print("Content-Type: text/html\r\n\r\n", end="", flush=True)
    print("<html><body>", flush=True)
    print("<h1>Modern File Upload Results</h1>", flush=True)

    # Get environment variables
    content_length_str = os.environ.get("CONTENT_LENGTH", "0")
    try:
        content_length = int(content_length_str)
    except ValueError:
        content_length = 0

    content_type = os.environ.get("CONTENT_TYPE", "")

    # Read raw binary data from stdin
    if content_length > 0:
        body_data = sys.stdin.buffer.read(content_length)
    else:
        body_data = b""

    # Parse payload using the email module
    raw_message = f"Content-Type: {content_type}\r\n\r\n".encode('utf-8') + body_data
    msg = email.message_from_bytes(raw_message)

    # Define target directory for uploads 
    # (This is relative to the directory where you start your webserv)
    upload_dir = os.environ.get("UPLOAD_DIR", "var/www/uploads")

    if msg.is_multipart():
        for part in msg.get_payload():
            name = part.get_param('name', header='content-disposition')
            filename = part.get_filename()
            
            # Extract the raw data payload
            payload = part.get_payload(decode=True)
            
            if filename:
                # 1. SECURITY: Prevent directory traversal attacks
                safe_filename = os.path.basename(filename)
                
                # 2. Create the full destination path
                save_path = os.path.join(upload_dir, safe_filename)
                
                print(f"<h2 style='color:green;'>File Uploaded: {safe_filename}</h2>", flush=True)
                print(f"<ul><li><b>Size:</b> {len(payload)} bytes</li></ul>", flush=True)
                
                # 3. Save the file to disk in binary write mode ('wb')
                try:
                    # Ensure directory exists before writing
                    if not os.path.exists(upload_dir):
                        os.makedirs(upload_dir)
                        
                    with open(save_path, 'wb') as f:
                        f.write(payload)
                    
                    print(f"<p style='color:blue;'><b>Success:</b> File saved permanently to <code>{save_path}</code></p>", flush=True)
                except Exception as e:
                    print(f"<p style='color:red;'><b>Error saving file:</b> {e}</p>", flush=True)
                    
            elif name:
                # Handles standard form text fields
                print(f"<h2>Form Field: {name}</h2>", flush=True)
                print(f"<p>Value: {payload.decode('utf-8', 'replace')}</p>", flush=True)
    else:
        print("<p style='color:red;'>No multipart data found in the body.</p>", flush=True)

    print("</body></html>", flush=True)

if __name__ == "__main__":
    main()
