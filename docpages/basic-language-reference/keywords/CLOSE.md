\page CLOSE CLOSE Keyword
```basic
CLOSE integer-variable
```

Closes an open file handle and flushes any pending writes.


\remark The parameter **must be an integer *variable*** that holds a valid file handle
\remark previously returned by \ref OPENIN "OPENIN",
\remark \ref OPENOUT "OPENOUT", or
\remark \ref OPENUP "OPENUP".
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
- Reading from or writing to a handle is only valid while it is open (see \ref EOF "EOF" for end-of-file checks).

**See also:**
\ref OPENIN "OPENIN", \ref OPENOUT "OPENOUT", \ref OPENUP "OPENUP", \ref WRITE "WRITE", \ref READ "READ$", \ref EOF "EOF"
