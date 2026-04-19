\page SEEK SEEK Keyword

```basic
SEEK <file handle>, <integer-position>
```

Seeks to the given offset in an open file. Note that if the position is outside the bounds of the file, or the handle is invalid an error will be thrown.

---

**See also:**
\ref OPENIN "OPENIN", \ref TELL "TELL"
