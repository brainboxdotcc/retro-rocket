\page SOCKBINWRITE SOCKBINWRITE Statement

```basic
SOCKBINWRITE(integer-socket, integer-buffer, integer-length)
```

Writes **binary data** from memory to a **connected socket**.

* `integer-socket` - socket file descriptor (e.g. from \ref CONNECT "CONNECT" or \ref SOCKACCEPT "SOCKACCEPT").
* `integer-buffer` - memory handle/pointer returned by \ref MEMALLOC "MEMALLOC".
* `integer-length` - number of bytes to write.

---

### Examples

```basic
size = FILESIZE(file$)
buf  = MEMALLOC(size)
BINREAD fh, buf, size

SOCKWRITE client, "Content-Length: "; size; CHR$(13); CHR$(10); CHR$(13); CHR$(10);
SOCKBINWRITE client, buf, size
SOCKFLUSH client

MEMRELEASE buf
```

---

### Notes

* Intended for sending file contents or other raw buffers.
* Socket must be connected; use \ref SOCKSTATUS "SOCKSTATUS" to check.
* May write less than requested if the peer closes; handle errors accordingly.

---

**See also:**
\ref SOCKREAD "SOCKREAD" · \ref SOCKWRITE "SOCKWRITE" · \ref SOCKSTATUS "SOCKSTATUS" · \ref BINREAD "BINREAD" · \ref MEMALLOC "MEMALLOC"
