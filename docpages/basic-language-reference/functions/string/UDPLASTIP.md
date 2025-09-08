\page UDPLASTIP UDPLASTIP$ Function
```basic
IP$ = UDPLASTIP$
```

Returns the **source IP address** (as a dotted-quad string) of the **most recent** packet returned by `UDPREAD$`.

- If no packet has been read yet, returns `"0.0.0.0"`.

---

### How to read it

- Call **after** `UDPREAD$` to know who sent the payload you just received.

---

### Examples
```basic
UDPBIND "0.0.0.0", 8080
S$ = UDPREAD$(8080)
IF LEN(S$) > 0 THEN
    PRINT UDPLASTIP$
END IF
```

---

### Notes
- The value updates only when `UDPREAD$` dequeues a packet.

**See also:**  
[UDPREAD$](https://github.com/brainboxdotcc/retro-rocket/wiki/UDPREAD) · \ref UDPLASTSOURCEPORT "UDPLASTSOURCEPORT"