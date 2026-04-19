\page MEMCOPY MEMCOPY Keyword

```basic
MEMCOPY source-integer-expression, dest-integer-expression, length-integer-expression
```

Copies a block of memory from the source address (usually allocated by \ref MEMALLOC "MEMALLOC" or \ref MEMREALLOC "MEMREALLOC") to a destination address (also usually allocated the same way).
Exactly `length-integer-expression` will be copied.

\warning This keyword does not allow for overlapping memory regions. For overlapping memory regions, use \ref MEMMOVE "MEMMOVE"

---

### Examples

```basic
buf = MEMALLOC(65536)
buf2 = MEMALLOC(65536)
MEMCOPY buf, buf2, 65536
MEMRELEASE buf
MEMRELEASE buf2
```

---

**See also:**
\ref MEMALLOC "MEMALLOC"
