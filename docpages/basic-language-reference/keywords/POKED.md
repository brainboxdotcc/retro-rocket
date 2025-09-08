\page POKED POKED Keyword
```basic
POKED address, value
```

Stores a **32-bit double-word** (low 32 bits of `value`) at `address` in **little-endian** order.


\remark The 4-byte span must be writable or `Bad Address at <address>` is raised.

---

### How to read it

- Typical width for device registers and descriptors.

---

### Examples
```basic
REM Store a 32-bit value
POKED &100300, &89ABCDEF
PRINT PEEKD(&100300)
```

---

### Notes
- Some devices use write-one-to-clear or posted writes; check the datasheet.

**See also:**  
\ref PEEKD "PEEKD" · \ref POKEW "POKEW" · \ref POKEQ "POKEQ"