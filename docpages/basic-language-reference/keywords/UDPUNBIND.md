\page UDPUNBIND UDPUNBIND Keyword
```basic
UDPUNBIND string-expression, port
```

Stops receiving UDP datagrams on a previously bound `port`. Any queued packets for that port are no longer delivered.

- `string-expression` is the local IP address used at bind time (currently **ignored**).
- `port` must be in the range `0` to `65535`.


@note If `port` is outside the valid range, an error is raised: `Invalid UDP port number`.

---

### How to read it

- Use `UDPUNBIND` to stop delivery when you no longer want to read from the port.

---

### Examples
```basic
UDPUNBIND "0.0.0.0", 8080
```

---

### Notes
- Unbinding while packets are still queued discards them from further reads.

**See also:**  
\ref UDPBIND "UDPBIND" · [UDPREAD$](https://github.com/brainboxdotcc/retro-rocket/wiki/UDPREAD%24)