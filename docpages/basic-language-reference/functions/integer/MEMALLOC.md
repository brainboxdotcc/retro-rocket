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
* The allocated memory is only valid while the program that requested it is running. The returned value ma be passed to other programs, however be aware of this restriction.
* Returned handle is suitable for binary I/O (\ref BINREAD "BINREAD", \ref BINWRITE "BINWRITE", \ref SOCKBINWRITE "SOCKBINWRITE", \ref SOCKBINREAD "SOCKBINREAD").
* Once the program ends, all memory requested by MEMALLOC is automatically freed.

**See also:**
\ref MEMRELEASE "MEMRELEASE" · \ref BINREAD "BINREAD" · \ref BINWRITE "BINWRITE"

