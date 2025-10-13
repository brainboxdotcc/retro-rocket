\page ssltest ssltest command

## ssltest

Demonstrates making a secure HTTPS connection using SSL/TLS.
It connects to a test host, sends a simple HTTP request, and prints the server’s reply.

### What it does
- Resolves the hostname `neuron.brainbox.cc`.
- Connects to port 443 using `SSLCONNECT`.
- Sends a minimal HTTP/1.0 request.
- Prints the response to the screen.
- Closes the connection.

### Usage
```
ssltest
```

\image html ssltest.png

### Notes
- This is a demo program, not a full web client.
- The output is raw HTTP headers and body.
- Useful for checking that SSL support and certificates are working correctly.
