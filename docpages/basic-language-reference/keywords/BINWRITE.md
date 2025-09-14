\page BINWRITE BINWRITE Statement

```basic
BINWRITE(integer-handle, integer-buffer, integer-length)
```

Writes **binary data** from memory to an **open file**.

* `integer-handle` — file handle from \ref OPENOUT "OPENOUT" or \ref OPENUP "OPENUP".
* `integer-buffer` — source memory handle/pointer (from \ref MEMALLOC "MEMALLOC" or other).
* `integer-length` — number of bytes to write.

---

### Examples

```basic
fh  = OPENOUT("copy.bin")
buf = MEMALLOC(1024)
REM ... fill buf with data ...
BINWRITE fh, buf, 1024
CLOSE fh
MEMRELEASE buf
```

---

### Notes

* Binary-safe; writes bytes exactly as stored in memory.
* Ensure the file was opened with appropriate mode (\ref OPENOUT "OPENOUT" to create/overwrite, \ref OPENUP "OPENUP" to update).

---

**See also:**
\ref OPENOUT "OPENOUT" · \ref OPENUP "OPENUP" · \ref MEMALLOC "MEMALLOC" · \ref MEMRELEASE "MEMRELEASE"
