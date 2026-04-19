\page MEMREALLOC MEMREALLOC Function

```basic
new-handle = MEMREALLOC(existing-handle, new-size)
```

Extends or shrinks an existing block of **heap memory** allocated by this function or \ref MEMALLOC "MEMALLOC" to the new given size (in bytes) and returns a **pointer/integer handle** to the new area.

Contents in the existing area will be copied to the new area. It is not guaranteed that the new area will start at the same location as the old area or even overlap with it.

The old handle does not need to be manually freed with \ref MEMRELEASE "MEMRELEASE", as the new handle takes its place.

Upon failure, the original existing-handle will still be valid. An error will still be raised, which you will have to capture with `ON ERROR`.

---

### Examples

```basic
size = FILESIZE("/system/webserver/logo.png")
buf  = MEMALLOC(size)
buf = MEMREALLOC(buf, size + 16)
MEMRELEASE buf
```

---

### Notes

* Contents of any newly extended portions are unspecified on allocation.
* Contents of any portions outside of a shrunken area are lost
* Always pair with \ref MEMRELEASE "MEMRELEASE" to avoid leaks.
* The allocated memory is only valid while the program that requested it is running. The returned value may be passed to other programs, however be aware of this restriction.
* Returned handle is suitable for binary I/O (\ref BINREAD "BINREAD", \ref BINWRITE "BINWRITE", \ref SOCKBINWRITE "SOCKBINWRITE", \ref SOCKBINREAD "SOCKBINREAD").
* Once the program ends, all memory requested by MEMALLOC is automatically freed.

**See also:**
\ref MEMRELEASE "MEMRELEASE" · \ref BINREAD "BINREAD" · \ref BINWRITE "BINWRITE"

