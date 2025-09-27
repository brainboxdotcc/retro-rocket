\page SSLSOCKACCEPT SSLSOCKACCEPT Function

```basic
client = SSLSOCKACCEPT(server, cert$, key$)
```

Removes and returns the next **established TLS connection** from the listening socket’s pending queue.
The connection is immediately upgraded to SSL/TLS using the supplied certificate and key.

Returns a non-negative file descriptor on success, or `-1` if no connection is ready (or on error).

@note `SOCKACCEPT` is **non-blocking**: it returns immediately. If there are no connections to accept, it will return `-1`.

---

### How to use it

* `server` is the file descriptor returned by `SOCKLISTEN(ip$, port, backlog)`.
* `cert$` is the path to the server’s certificate file.
* `key$` is the path to the server’s private key file.
* Call repeatedly in your main loop to accept clients as they become ready.
* Use the returned `client` descriptor with `SOCKWRITE`, `SOCKREAD`, `INSOCKET$`, and `SOCKCLOSE`.

---

### Examples

```basic
REM Secure echo server
server = SOCKLISTEN(NETINFO$("ip"), 4433, 5)
IF server < 0 THEN PRINT "Listen failed"
PRINT "Listening on port 4433 (TLS)"

REPEAT
    client = SSLSOCKACCEPT(server, "/system/ssl/server.crt", "/system/ssl/server.key")
    IF client >= 0 THEN
        SOCKREAD client, REQ$
        PRINT "Got request: "; REQ$
        SOCKWRITE client, "Secure reply"
        SOCKCLOSE client
    END IF
UNTIL INKEY$ <> ""

SOCKCLOSE server
```

---

### Notes

* The certificate and key must match and be valid, in standard PEM format.
* Uses standard TLS (commonly referred to as “SSL”) with certificate-based encryption.
* Returns `-1` when no connection is available; try again later.
* Each successful call returns a **new** secure client descriptor; the server remains listening.

**See also:**
\ref SOCKLISTEN "SOCKLISTEN" · \ref SOCKWRITE "SOCKWRITE" · \ref SOCKREAD "SOCKREAD" · \ref INSOCKET "INSOCKET$" · \ref SOCKSTATUS "SOCKSTATUS" · \ref SOCKCLOSE "SOCKCLOSE" · \ref SSLCONNECT "SSLCONNECT"
