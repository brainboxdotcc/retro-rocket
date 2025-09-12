\page SOCKACCEPT SOCKACCEPT Function

```basic
client = SOCKACCEPT(server)
```

Removes and returns the next **established** connection from the listening socket’s pending queue. Returns a non-negative file descriptor on success, or `-1` if no connection is ready (or on error).

@note `SOCKACCEPT` is **non-blocking**: it returns immediately. If there are no connections to accept, it will return `-1`.

---

### How to use it

* `server` is the file descriptor returned by `SOCKLISTEN(ip$, port, backlog)`.
* Call repeatedly in your main loop to accept clients as they become ready.
* Use the returned `client` descriptor with `SOCKWRITE`, `SOCKREAD`, `INSOCKET$`, and `SOCKCLOSE`.
* The pending queue is FIFO; only the head is checked each call.

---

### Examples

```basic
REM Minimal accept loop
server = SOCKLISTEN(NETINFO$("ip"), 2000, 5)
IF server < 0 THEN PRINT "Listen failed": END
PRINT "Listening on port 2000"

REPEAT
    client = SOCKACCEPT(server)
    IF client >= 0 THEN
        SOCKWRITE client, "HELLO!" + CHR$(13) + CHR$(10)
        SOCKCLOSE client
    END IF
UNTIL INKEY$ <> ""

SOCKCLOSE server
```

---

### Notes

* Returns `-1` when no established connection is available; try again later.
* Each successful call returns a **new** client descriptor; the server remains listening.

**See also:**
\ref SOCKLISTEN "SOCKLISTEN" · \ref SOCKWRITE "SOCKWRITE" · \ref SOCKREAD "SOCKREAD" · \ref INSOCKET\$ "INSOCKET\$" · \ref SOCKSTATUS "SOCKSTATUS" · \ref SOCKCLOSE "SOCKCLOSE" · \ref NETINFO\$ "NETINFO\$"
