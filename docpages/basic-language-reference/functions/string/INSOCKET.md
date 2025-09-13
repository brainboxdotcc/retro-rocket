\page INSOCKET INSOCKET$ Function

```basic
INSOCKET$(integer-expression)
```

Reads and returns a **single character** from the socket identified by the given socket descriptor.

* If there is no data available to read, an **empty string** is returned.
* If the socket has closed or encountered an error, an error is raised describing the socket failure.

---

### Examples

```basic
REM Read characters from a connected socket
REPEAT
    c$ = INSOCKET$(fd)
    IF c$ > "" THEN
        PRINT "Received: "; c$
    ENDIF
UNTIL SOCKSTATUS(fd) = FALSE
```

```basic
REM Assemble incoming data into a string
line$ = ""
REPEAT
    c$ = INSOCKET$(fd)
    IF c$ > "" THEN line$ = line$ + c$
UNTIL c$ = "" OR SOCKSTATUS(fd) = FALSE
PRINT "Line: "; line$
```

---

### Notes

* Operates at the **character level**; repeated calls are required to read longer strings.
* Non-blocking: if no data is present, returns immediately with `""`.
* If the socket is closed, further calls will raise an error.
* To check connection state, use \ref SOCKSTATUS "SOCKSTATUS".

---

**See also:**
\ref SOCKREAD "SOCKREAD" · \ref SOCKWRITE "SOCKWRITE" · \ref SOCKSTATUS "SOCKSTATUS"
