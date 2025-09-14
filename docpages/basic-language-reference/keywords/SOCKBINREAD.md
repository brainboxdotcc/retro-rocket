\page SOCKBINREAD SOCKBINREAD Statement

```basic
SOCKBINREAD(integer-socket, integer-buffer, integer-length)
```

Reads **binary data** from a **connected socket** into memory.

* `integer-socket` — socket file descriptor.
* `integer-buffer` — destination memory handle/pointer (from \ref MEMALLOC "MEMALLOC").
* `integer-length` — maximum number of bytes to read.

---

### Examples

```basic
buf = MEMALLOC(4096)
SOCKBINREAD client, buf, 4096
REM ... process buffer ...
MEMRELEASE buf
```

---

### Notes

* Reads up to the specified number of bytes into the buffer.
* Use \ref SOCKSTATUS "SOCKSTATUS" to detect disconnects.
* Pair with \ref MEMALLOC "MEMALLOC" / \ref MEMRELEASE "MEMRELEASE" for buffer management.

---

**See also:**
\ref SOCKREAD "SOCKREAD" · \ref SOCKWRITE "SOCKWRITE" · \ref MEMALLOC "MEMALLOC" · \ref MEMRELEASE "MEMRELEASE"
