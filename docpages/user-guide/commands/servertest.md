\page servertest servertest command

## servertest

Starts a very simple TCP socket server on port **2000**.
It accepts connections and replies with a cheerful greeting before closing.

### What it does
- Listens for incoming connections on the machine’s IP address, port 2000.
- For each connection:
  - Prints “Handling connection, HELLORLD!” to the screen.
  - Sends the string `HELLORLD!` back to the client.
  - Closes the connection.
- Runs in a loop until you press a key.

### Usage
```
servertest
```

### Example session
```
servertest
Socket server listening on port 2000
Handling connection, HELLORLD!
Handling connection, HELLORLD!
```

(Client side output will just see the text `HELLORLD!` returned.)

### Notes
- Intended as a simple demo of networking in Retro Rocket.
- Only one line of text is returned; it’s not an interactive service.
- Stops cleanly when any key is pressed.
