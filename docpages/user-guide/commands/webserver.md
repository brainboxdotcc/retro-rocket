\page webserver webserver command

## webserver

Starts a very simple built-in HTTP server on port 80.
It serves files from the `/system/webserver` directory.

### What it does
- Listens for connections on the machine’s IP address, port 80.
- For each incoming request:
  - Reads the HTTP request.
  - If the request path is `/`, it serves `/system/webserver/index.html`.
  - Otherwise, it tries to serve the requested file from `/system/webserver`.
  - Sends `200 OK` with the file contents if found.
  - Sends `404 Not Found` if the file does not exist.
- Closes connections after each request.
- Runs until a key is pressed at the console.

### Usage
```
webserver
```

### Example
```
webserver
Web server running. Press any key to terminate.
```

Now open a web browser on another machine and navigate to: http://<retro-rocket-ip>/

You will see the contents of `/system/webserver/index.html`.

### Notes
- Files are served directly from `/system/webserver` and its subdirectories.
- Only static files are supported (HTML, images, plain text, etc.).
- No directory listing is generated if you request a folder.
- Press any key in the console to stop the server.
- Port 80 must be free; only one service can listen there at a time.
- With the standard packaged qemu VM, port 2080 is forwarded to port 80 within the VM. You may test the web server via that port instead.
