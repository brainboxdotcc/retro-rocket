\page UDPWRITE UDPWRITE Keyword
```basic
UDPWRITE dest-ip$, source-port, dest-port, data$
```

Sends a UDP datagram to `dest-ip$ : dest-port` with payload `data$`, using `source-port` as the local port.

- `source-port` and `dest-port` must be in `0` to `65535`.
- `data$` must be between **1** and **65530** characters.


@note Invalid port numbers raise: `Invalid UDP port number`.
@note Invalid payload size raises: `Invalid UDP packet length`.

---

### How to read it

- Provide a dotted-quad destination IP (e.g. `"127.0.0.1"`).
- Use together with `UDPBIND`/`UDPREAD$` for simple local tests.

---

### Examples
```basic
UDPWRITE "127.0.0.1", 1234, 8080, "hello"
```

```basic
UDPBIND "0.0.0.0", 8080
UDPWRITE "127.0.0.1", 1234, 8080, "ping"
PRINT UDPREAD$(8080)
```

---

### Notes
- The payload is transmitted exactly as provided in `data$`.
- Ensure the destination host and port are reachable on your network.

**See also:**  
\ref UDPBIND "UDPBIND" · [UDPREAD$](https://github.com/brainboxdotcc/retro-rocket/wiki/UDPREAD)