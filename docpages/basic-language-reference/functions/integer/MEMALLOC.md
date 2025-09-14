\page MEMALLOC MEMALLOC Function

```basic
MEMALLOC(integer-expression)
```

Allocates a block of **heap memory** of the given size (in bytes) and returns a **pointer/integer handle** to it.

---

### Examples

```basic
size = FILESIZE("/system/webserver/logo.png")
buf  = MEMALLOC(size)
REM ... use buf with BINREAD / SOCKBINWRITE ...
MEMRELEASE buf
```

---

### Notes

* Contents are unspecified on allocation.
* Always pair with \ref MEMRELEASE "MEMRELEASE" to avoid leaks.
* Returned handle is suitable for binary I/O (\ref BINREAD "BINREAD", \ref BINWRITE "BINWRITE", \ref SOCKBINWRITE "SOCKBINWRITE", \ref SOCKBINREAD "SOCKBINREAD").

**See also:**
\ref MEMRELEASE "MEMRELEASE" · \ref BINREAD "BINREAD" · \ref BINWRITE "BINWRITE"

