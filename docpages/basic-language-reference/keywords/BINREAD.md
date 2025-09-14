\page BINREAD BINREAD Statement

```basic
BINREAD(integer-handle, integer-buffer, integer-length)
```

Reads **binary data** from an **open file** into memory.

* `integer-handle` — file handle from \ref OPENIN "OPENIN" or \ref OPENUP "OPENUP".
* `integer-buffer` — destination memory handle/pointer (from \ref MEMALLOC "MEMALLOC").
* `integer-length` — number of bytes to read.

---

### Examples

```basic
fh  = OPENIN("asset.bin")
size = FILESIZE("asset.bin")
buf  = MEMALLOC(size)

BINREAD fh, buf, size

REM ... use buffer ...
MEMRELEASE buf
CLOSE fh
```

---

### Notes

* Use \ref FILESIZE "FILESIZE" to size your buffer appropriately.
* Binary-safe (no line translation).
* Reading past end of file yields fewer bytes or an error, depending on context; check with \ref EOF "EOF" as needed.

---

**See also:**
\ref OPENIN "OPENIN" · \ref OPENUP "OPENUP" · \ref FILESIZE "FILESIZE" · \ref EOF "EOF" · \ref MEMALLOC "MEMALLOC"
