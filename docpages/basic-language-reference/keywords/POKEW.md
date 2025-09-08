\page POKEW POKEW Keyword
```basic
POKEW address, value
```

Stores a **16-bit word** (low 16 bits of `value`) at `address` in **little-endian** order.


\remark The 2-byte span must be writable or `Bad Address at <address>` is raised.

---

### How to read it

- Writes `value AND &FFFF` to `address` and `address+1`.

---

### Examples
```basic
REM Store a 16-bit word
POKEW &100200, &1234
PRINT PEEKW(&100200)
```

---

### Notes
- Match the width to the device’s documented register size.

**See also:**  
\ref POKE "POKE" · \ref POKED "POKED" · \ref POKEQ "POKEQ" · \ref PEEKW "PEEKW"