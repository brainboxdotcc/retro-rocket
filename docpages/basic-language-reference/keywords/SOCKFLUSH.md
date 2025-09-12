\page SOCKFLUSH SOCKFLUSH Keyword

```basic
SOCKFLUSH integer-variable
```

Blocks until any **pending outbound data** for the given TCP socket has been **flushed from the BASIC send buffer** to the TCP stack.
Use after \ref SOCKWRITE "SOCKWRITE" when you need to ensure queued bytes have been handed off before proceeding (e.g. before closing the socket).

@note This waits for the **local send buffer to empty**; it does **not** wait for peer acknowledgements.

---

### How to use it

* `integer-variable` must contain a socket handle returned by \ref CONNECT "CONNECT" or \ref SOCKACCEPT "SOCKACCEPT".
* The statement **yields** like \ref SOCKREAD "SOCKREAD": your program pauses and resumes automatically when the condition is met.
* Typical use is `SOCKWRITE` → `SOCKFLUSH` → `SOCKCLOSE`.

@note Press `CTRL+ESC` at any time to **cancel** waiting.
@note Without an error handler, the program **terminates** on errors; with `ON ERROR`, control passes to your handler.

---

### Examples

**Send a line, flush, then close**

```basic
server = SOCKLISTEN(NETINFO$("ip"), 2000, 5)
client = SOCKACCEPT(server)
IF client >= 0 THEN
    SOCKWRITE client, "HELLORLD!" + CHR$(13) + CHR$(10)
    SOCKFLUSH client
    SOCKCLOSE client
END IF
SOCKCLOSE server
```

**Ensure data is flushed before reusing variables**

```basic
CONNECT s, "93.184.216.34", 80
SOCKWRITE s, "HEAD / HTTP/1.0" + CHR$(13) + CHR$(10) + CHR$(13) + CHR$(10)
SOCKFLUSH s
SOCKCLOSE s
```

---

### Notes

* If the socket has no pending data, `SOCKFLUSH` returns immediately.
* If the connection is no longer valid (e.g. already closed), `SOCKFLUSH` returns without waiting.
* Use \ref SOCKSTATUS "SOCKSTATUS" to query whether a socket is connected.

**See also:**
\ref SOCKWRITE "SOCKWRITE" ·
\ref SOCKREAD "SOCKREAD" ·
\ref SOCKCLOSE "SOCKCLOSE" ·
\ref CONNECT "CONNECT" ·
\ref SOCKACCEPT "SOCKACCEPT" ·
\ref SOCKLISTEN "SOCKLISTEN"
