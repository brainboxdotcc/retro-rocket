\page SOCKLISTEN SOCKLISTEN Function

```basic
FD = SOCKLISTEN(ip$, port, backlog)
```

Creates a **listening TCP socket** bound to `ip$:port` and starts queueing incoming connections (up to `backlog`). Returns a non-negative file descriptor on success, or `-1` on failure.

---

### How to use it

* `ip$` is a dotted-quad string (e.g. `"10.0.2.15"`). Use `NETINFO$("ip")` for the local address.
* `port` is the local TCP port to listen on.
* `backlog` is the maximum number of pending (not yet accepted) connections to queue.
* Accept connections with `SOCKACCEPT(FD)`; write with `SOCKWRITE`, read with `SOCKREAD`/`INSOCKET$`, and close with `SOCKCLOSE`.

---

### Examples

```basic
REM Listen and handle one connection
srv = SOCKLISTEN(NETINFO$("ip"), 2000, 5)
PRINT "Socket server listening on port 2000"
REPEAT
    cli = SOCKACCEPT(srv)
    IF cli >= 0 THEN
        SOCKWRITE cli, "HELLORLD!" + CHR$(10) + CHR$(13)
        SOCKFLUSH cli
        SOCKCLOSE cli
    ENDIF
UNTIL INKEY$ <> ""
SOCKCLOSE srv
```

---

### Notes

* Binding to `"0.0.0.0"` (if supported) listens on all local interfaces.
* `SOCKACCEPT` returns a new FD for each established connection; call repeatedly to drain the queue.

**See also:**
\ref SOCKACCEPT "SOCKACCEPT" · \ref SOCKWRITE "SOCKWRITE" · \ref SOCKREAD "SOCKREAD" · \ref INSOCKET\$ "INSOCKET\$" · \ref SOCKCLOSE "SOCKCLOSE" · \ref NETINFO\$ "NETINFO\$"
