\page SOCKREAD SOCKREAD Keyword
```basic
SOCKREAD integer-variable, variable
```

Reads a value from an **open TCP socket** into `variable`.  
The first parameter must be an **integer variable** containing the socket handle returned by \ref CONNECT "CONNECT".

The **semantics match** \ref INPUT "INPUT":

- The call is **blocking**: it waits until there is **enough data** to parse a valid value for the destination type.
- `variable` may be **string**, **integer**, or **real**.
- If the incoming data does **not** form a valid number when an integer or real is expected, the destination is set to **0**.
- Unlike `INPUT`, there is **no echo** (data comes from the socket, not the keyboard).


@note Press `CTRL+ESC` at any time to **cancel** waiting for socket input.
@note Without an error handler, the program **terminates**.
@note With an `ON ERROR` handler, control passes there instead.

---

### Examples

**Read a text line**
```basic
CONNECT S, "93.184.216.34", 80
REM Ask the server for a line, then read it
SOCKWRITE S, "HEAD / HTTP/1.0" + CHR$(13) + CHR$(10) + CHR$(13) + CHR$(10)
SOCKREAD S, LINE$
PRINT LINE$
SOCKCLOSE S
```

**Read a number**
```basic
CONNECT S, "192.0.2.10", 1234
REM Peer will send an integer in ASCII, e.g. 42 followed by newline
SOCKREAD S, N
IF N = 0 THEN
    PRINT "Invalid or zero value received"
ELSE
    PRINT "Got: "; N
ENDIF
```

**Handle cancellation and errors**
```basic
ON ERROR PROCnet_err

CONNECT S, "203.0.113.5", 7000
IF S >= 0 THEN
    PRINT "Waiting for data (CTRL+ESC to cancel)..."
    SOCKREAD S, DATA$
    PRINT "Received: "; DATA$
    SOCKCLOSE S
ENDIF
END

DEF PROCnet_err
    PRINT "Socket error: "; ERR$
    ON ERROR PROCnet_err
ENDPROC
```

---

### Notes
- Treat the socket as a **byte stream**; if you need to read multiple fields on one line, call `SOCKREAD` multiple times or define a simple protocol (for example, one value per line).
- After `SOCKCLOSE`, the handle is invalid for further `SOCKREAD`/`SOCKWRITE`.
- Use \ref DNS "DNS$" to resolve hostnames to IP strings for `CONNECT`.

**See also:**  
\ref CONNECT "CONNECT" ·
\ref SOCKWRITE "SOCKWRITE" ·
\ref SOCKCLOSE "SOCKCLOSE" ·
\ref INPUT "INPUT"
