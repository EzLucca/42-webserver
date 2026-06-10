#!/usr/bin/env python3
import os
import json

def main():
    # 1. Print the required CGI headers (Notice it is application/json!)
    print("Content-Type: application/json\r\n\r\n", end="", flush=True)

    # 2. Tell Python where to look (Use the environment variable if C++ sent it!)
    upload_dir = "var/www/uploads"
    
    image_files = []
    
    # 3. Safely scan the directory
    if os.path.exists(upload_dir):
        for filename in os.listdir(upload_dir):
            # Only grab actual images
            if filename.lower().endswith(('.png', '.jpg', '.jpeg', '.gif')):
                image_files.append(filename)
    
	# Add a debug_path to the JSON so we can see exactly where Python is looking
    json_output = json.dumps({
        "images": image_files, 
        "debug_path": os.path.abspath(upload_dir)
    })
    print(json_output, flush=True)

if __name__ == "__main__":
    main()
