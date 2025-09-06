\page CONNECT CONNECT Keyword
```basic
CONNECT integer-variable, string-expression, integer-expression
```

Opens a **TCP connection**.

- **First parameter**: an **integer variable** (created if it does not yet exist).  
  On success it will be set to a **non-negative handle** that you then pass to
  [`SOCKREAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/SOCKREAD),
  [`SOCKWRITE`](https://github.com/brainboxdotcc/retro-rocket/wiki/SOCKWRITE),
  and [`SOCKCLOSE`](https://github.com/brainboxdotcc/retro-rocket/wiki/SOCKCLOSE).
- **Second parameter**: a **string** containing the IP address to connect to.  
  To resolve a hostname into an address string, use [`DNS$`](https://github.com/brainboxdotcc/retro-rocket/wiki/DNS).
- **Third parameter**: the **port number** (integer expression).

On **failure**, an error is raised.

---

##### Example: connect, send, receive, close

```basic
HOST$ = DNS$("example.com")
CONNECT H, HOST$, 80
SOCKWRITE H, "HEAD / HTTP/1.0" + CHR(13) + CHR(10) + CHR(13) + CHR(10)
SOCKREAD H, REPLY$
PRINT REPLY$
SOCKCLOSE H
```

---

##### Notes
- The first argument **must be an integer variable**, not a literal or expression.
- Always close the handle with `SOCKCLOSE` when finished.
- `SOCKREAD` is a **blocking** operation; see its page for how to cancel with `CTRL+ESC`.

**See also:** [`DNS$`](https://github.com/brainboxdotcc/retro-rocket/wiki/DNS), [`SOCKREAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/SOCKREAD), [`SOCKWRITE`](https://github.com/brainboxdotcc/retro-rocket/wiki/SOCKWRITE), [`SOCKCLOSE`](https://github.com/brainboxdotcc/retro-rocket/wiki/SOCKCLOSE), [`SOCKSTATUS`](https://github.com/brainboxdotcc/retro-rocket/wiki/SOCKSTATUS)
