\page MEMRELEASE MEMRELEASE Keyword

```basic
MEMRELEASE integer-expression
```

Releases (frees) a block of memory previously obtained via \ref MEMALLOC "MEMALLOC".

---

### Examples

```basic
buf = MEMALLOC(65536)
REM ... use buf ...
MEMRELEASE buf
buf = 0
```

Attempting to release the same allocation twice raises an error:

```basic
buf = MEMALLOC(1024)
MEMRELEASE buf
MEMRELEASE buf
```

Attempting to release an address that was not returned directly by \ref MEMALLOC "MEMALLOC" also raises an error:

```basic
buf = MEMALLOC(1024)
REM Invalid: interior pointer
MEMRELEASE buf + 100
```

---

### Notes

* Passing an invalid, already-freed, or non-allocated address raises an error.
* `MEMRELEASE` only accepts addresses returned directly by \ref MEMALLOC "MEMALLOC".
* Interior pointers such as `buf + 1` are rejected.
* Double-free attempts are detected and rejected safely.
* After releasing, set your variable to `0` if you want to avoid accidental reuse.
* Your program ending releases all memory it requested using \ref MEMALLOC "MEMALLOC" - be aware of this if you pass references to this memory to a child program.

---

**See also:**
\ref MEMALLOC "MEMALLOC"
