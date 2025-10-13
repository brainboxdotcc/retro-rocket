\page udptest udptest command

## udptest

Demonstrates sending and receiving data using UDP sockets.
It binds to a local port, sends a packet, waits for a response, and prints the result.

### What it does
- Binds to `127.0.0.1:9999`.
- Sends the string `"HI"` to port 9999.
- Waits until a packet is received.
- Prints the source IP, source port, and the packet contents.

### Usage
```
udptest
```

### Example output
```
BIND
WRITE
READ
GET IP
GET PORT
9999 127.0.0.1 HI
```

### Notes
- This is a self-test: it sends to and reads from the same local port.
- Useful as a demo of UDP networking functions in Retro Rocket BASIC.
- Output will vary if packets are received from other hosts or ports.
