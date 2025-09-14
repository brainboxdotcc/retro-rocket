\page MEMUSED MEMUSED Function

```basic
MEMUSED
```

Returns the total **memory in use** (bytes) by **all BASIC programs and the OS**.

\image html memory.png

### Examples

```basic
PRINT MEMUSED
PRINT "Used  = "; MEMUSED / (1024 * 1024); " MB"
PRINT "Free  = "; (MEMORY - MEMUSED) / (1024 * 1024); " MB"
PRINT "Used % = "; (MEMUSED * 100) / MEMORY
```

Example on a 4 GB machine with \~200 MB used:

```
Used  = 200 MB
Free  = 3896 MB
Used % = 4
```

### Notes

* Bytes, 64-bit integer.
* Includes kernel, caches (block cache, framebuffers, network buffers), plus all BASIC programs.
* Identity: `MEMFREE + MEMUSED = MEMORY`.

**See also:**
\ref MEMORY "MEMORY" · \ref MEMFREE "MEMFREE" · \ref MEMPEAK "MEMPEAK" · \ref MEMPROGRAM "MEMPROGRAM" · \ref MEMORY "MEMRELEASE" · \ref MEMORY "MEMALLOC"
