\page MEMRELEASE MEMRELEASE Function

```basic
MEMRELEASE(integer-expression)
```

Releases (frees) a block of memory previously obtained via \ref MEMALLOC "MEMALLOC".

---

### Examples

```basic
buf = MEMALLOC(65536)
REM ... use buf ...
MEMRELEASE buf
```

---

### Notes

* Passing an invalid or already-freed handle raises an error.
* After releasing, set your variable to `0` if you want to avoid accidental reuse.
* Your program ending releases all memory it requested using \ref MEMALLOC - be aware of this if you pass reference of this memory to a child program!

---

**See also:**
\ref MEMALLOC "MEMALLOC"
