\page DNS DNS$ Function

```basic
DNS$(string-expression)
```

Resolves a **hostname** into an **IP address string**.
If the hostname cannot be resolved, an error is raised.

---

### Examples

```basic
PRINT DNS$("retrorocket.dev")
```

Might produce:

```
141.95.6.222
```

```basic
REM Use DNS$ before connecting a socket
host$ = "irc.libera.chat"
ip$ = DNS$(host$)
PRINT "Connecting to "; host$; " ("; ip$; ")"
CONNECT fd, ip$, 6667
```

---

### Notes

* Always returns an IPv4 address in dotted decimal format (`a.b.c.d`).
* Name resolution is performed via the operating system’s network stack.
* Failures (e.g. host not found, no network) return the value "0.0.0.0"

---

**See also:**
\ref CONNECT "CONNECT" · \ref SOCKSTATUS "SOCKSTATUS"
