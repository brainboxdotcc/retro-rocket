\page UDPLASTSOURCEPORT UDPLASTSOURCEPORT Keyword
```basic
P = UDPLASTSOURCEPORT
```

Returns the **source port** of the **most recent** packet returned by `UDPREAD$`.

- If no packet has been read yet, returns `0`.

---

### How to read it

- Call **after** `UDPREAD$` to get the sender’s port for the last payload.

---

### Examples
```basic
UDPBIND "0.0.0.0", 8080
S$ = UDPREAD$(8080)
IF LEN(S$) > 0 THEN
    PRINT UDPLASTSOURCEPORT
END IF
```

---

### Notes
- The value updates only when `UDPREAD$` dequeues a packet.

**See also:**  
[UDPREAD$](https://github.com/brainboxdotcc/retro-rocket/wiki/UDPREAD%24) · [UDPLASTIP$](https://github.com/brainboxdotcc/retro-rocket/wiki/UDPLASTIP%24)