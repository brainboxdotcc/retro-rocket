\page UDPBIND UDPBIND Keyword
```basic
UDPBIND string-expression, port
```

Binds the **UDP input** of the BASIC runtime to a local `port`. Incoming UDP datagrams to this port are queued for later retrieval with `UDPREAD$`.

- `string-expression` is the local IP address to bind to (currently **ignored**; binding is by `port`).
- `port` must be in the range `0` to `65535`.


> If `port` is outside the valid range, an error is raised: `Invalid UDP port number`.

---

### How to read it

- Use `UDPBIND` before attempting to read with `UDPREAD$`.
- Each port has its own queue; packets are delivered in **first-in, first-out** order.

---

### Examples
```basic
UDPBIND "0.0.0.0", 8080
```

```basic
UDPBIND "127.0.0.1", 9000
```

---

### Notes
- Binding the same port more than once is not required; bind once, then read.
- To stop receiving on a port, use `UDPUNBIND`.

**See also:**  
[UDPUNBIND](https://github.com/brainboxdotcc/retro-rocket/wiki/UDPUNBIND) · [UDPREAD$](https://github.com/brainboxdotcc/retro-rocket/wiki/UDPREAD%24) · [UDPLASTIP$](https://github.com/brainboxdotcc/retro-rocket/wiki/UDPLASTIP%24) · [UDPLASTSOURCEPORT](https://github.com/brainboxdotcc/retro-rocket/wiki/UDPLASTSOURCEPORT)