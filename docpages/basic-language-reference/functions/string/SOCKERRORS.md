\page SOCKERRORS SOCKERROR$ Function

```basic
V$ = SOCKERROR$(socket-handle)
```

Returns a human-readable description of the last socket error associated with the given `socket-handle`.

If the socket has encountered an error or has been closed, the corresponding error string will be returned (for example, `"connection reset"`, `"aborted"`, `"timed out"`).

If no error has occurred, the string `"No error"` is returned.

**Notes**

* The error code is stored per file descriptor and remains available even after the connection has been closed.
* This allows error inspection after operations such as `CLOSE` or remote disconnection.
* The returned string is derived from the internal TCP error code.

**Errors**

* Returns `"invalid socket"` if the handle is out of range or not recognised.

**Examples**

```basic
CONNECT s, "example.com", 23

' ... use the socket ...

CLOSE s

PRINT SOCKERROR$(s)
```

```basic
CONNECT s, "example.com", 23

IF SOCKSTATUS(s) = FALSE THEN
    PRINT "Socket error: "; SOCKERROR$(s)
ENDIF
```

**See also:**

\ref CONNECT "CONNECT" · \ref CLOSE "CLOSE" · \ref SOCKSTATUS "SOCKSTATUS"
