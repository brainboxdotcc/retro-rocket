\page UDPREAD UDPREAD$ Function
```basic
S$ = UDPREAD$(port)
```

Dequeues the **next UDP packet** for `port` and returns its **payload** as a string.

- If the queue is empty, returns an **empty string** (`""`), sets the last-source details to `0.0.0.0` and `0`.
- On success, you can query the sender details with `UDPLASTIP$` and `UDPLASTSOURCEPORT`.


@note If `port` is outside `0` to `65535`, an error is raised: `Invalid UDP port number`.

---

### How to read it

- Call after `UDPBIND`.
- Each call removes **one** packet from the queue (FIFO).

---

### Examples
```basic
UDPBIND "0.0.0.0", 8080
S$ = UDPREAD$(8080)
IF LEN(S$) > 0 THEN
    PRINT "From "; UDPLASTIP$; ":"; UDPLASTSOURCEPORT
    PRINT "Data: "; S$
END IF
```

```basic
UDPBIND "0.0.0.0", 9000
S$ = UDPREAD$(9000)
PRINT S$
```

---

### Notes
- The returned string is the exact payload that was sent.
- `UDPLASTIP$` and `UDPLASTSOURCEPORT` always refer to the **most recent** packet returned by `UDPREAD$`.

**See also:**  
\ref UDPBIND "UDPBIND" · [UDPLASTIP$](https://github.com/brainboxdotcc/retro-rocket/wiki/UDPLASTIP%24) · \ref UDPLASTSOURCEPORT "UDPLASTSOURCEPORT" · \ref UDPWRITE "UDPWRITE"