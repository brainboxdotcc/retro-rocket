\page UDPLASTIP UDPLASTIP$ Function

```basic
UDPLASTIP$
```

Returns the **source IP address** (as a dotted-quad string) of the **most recent packet** returned by \ref UDPREAD "UDPREAD\$".

* If no packet has been read yet, returns `"0.0.0.0"`.

---

### How to read it

* Call **after** `UDPREAD$` to determine the sender of the payload you just received.

---

### Examples

```basic
UDPBIND "0.0.0.0", 8080
S$ = UDPREAD$(8080)
IF LEN(S$) > 0 THEN
    PRINT "Packet received from "; UDPLASTIP$
ENDIF
```

---

### Notes

* The value updates only when \ref UDPREAD "UDPREAD\$" dequeues a packet.
* Always returns the sender’s IPv4 address in dotted decimal form (`a.b.c.d`).

---

**See also:**
\ref UDPREAD "UDPREAD$" · \ref UDPLASTSOURCEPORT "UDPLASTSOURCEPORT"
