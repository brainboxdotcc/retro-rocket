\page SOCKSTATUS SOCKSTATUS Function

```basic
SOCKSTATUS(numeric-expression)
```

Checks the connection state of a **socket**.
Returns `TRUE` if the socket represented by the numeric expression (a handle returned by \ref CONNECT "CONNECT") is **connected**, or `FALSE` if it is not.

Note that a socket which is still in the process of connecting may return `FALSE`.

---

### Examples

```basic
REM Attempt to connect and wait until established
CONNECT fd, "example.com", 80
REPEAT
    PRINT "Waiting for connection..."
    WAIT 50
UNTIL SOCKSTATUS(fd) = TRUE
PRINT "Connected!"
```

```basic
REM Detect if a socket has dropped
IF SOCKSTATUS(fd) = FALSE THEN
    PRINT "Connection lost"
ENDIF
```

---

### Notes

* Returns a boolean value: `TRUE` (connected) or `FALSE` (not connected).
* A return of `FALSE` may indicate either “not yet connected” or “disconnected”.
* Use with \ref SOCKREAD "SOCKREAD", \ref SOCKWRITE "SOCKWRITE", and \ref SOCKCLOSE "SOCKCLOSE" for socket I/O.

---

**See also:**
\ref CONNECT "CONNECT" · \ref SOCKREAD "SOCKREAD" · \ref SOCKWRITE "SOCKWRITE" · \ref SOCKCLOSE "SOCKCLOSE"
