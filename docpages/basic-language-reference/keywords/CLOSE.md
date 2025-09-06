\page CLOSE CLOSE Keyword
```basic
CLOSE integer-variable
```

Closes an open file handle and flushes any pending writes.


\remark The parameter **must be an integer *variable*** that holds a valid file handle
\remark previously returned by [`OPENIN`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENIN),
\remark [`OPENOUT`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENOUT), or
\remark [`OPENUP`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENUP).
\remark A numeric literal or expression is **not** accepted.

---

##### Example

```basic
FH = OPENOUT("log.txt")
WRITE FH, "hello"
CLOSE FH
```

---

##### Notes
- After `CLOSE`, the handle becomes invalid and must not be reused unless re-opened.
- Closing an already-closed or never-opened handle raises a runtime error
  (trap with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR) if needed).
- All buffered data is written to storage before the handle is released.
- Reading from or writing to a handle is only valid while it is open (see [`EOF`](https://github.com/brainboxdotcc/retro-rocket/wiki/EOF) for end-of-file checks).

**See also:** [`OPENIN`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENIN), [`OPENOUT`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENOUT), [`OPENUP`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENUP), [`WRITE`](https://github.com/brainboxdotcc/retro-rocket/wiki/WRITE), [`READ$`](https://github.com/brainboxdotcc/retro-rocket/wiki/READ), [`EOF`](https://github.com/brainboxdotcc/retro-rocket/wiki/EOF)
