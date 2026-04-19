\page MEMSET MEMSET Keyword

```basic
MEMSET memory-handle-integer-expression, byte-value, length-integer-expression
```

Sets all byte values in the memory pointed to by `memory-handle-integer-expression` to `byte-value`, up to length `length-integer-expression`.

If `byte-value` is < 0  or > 255, an error will be raised.

---

### Examples

```basic
buf = MEMALLOC(65536)
MEMSET buf, 0, 65536
MEMRELEASE buf
```

---

**See also:**
\ref MEMALLOC "MEMALLOC"
