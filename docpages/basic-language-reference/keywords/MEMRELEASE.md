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

---

**See also:**
\ref MEMALLOC "MEMALLOC"
