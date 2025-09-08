\page PEEKW PEEKW Function
```basic
X = PEEKW(address)
```

Reads a **16-bit word** from `address` and returns it as an integer (0–65535).


@note The full 2-byte span must be valid and mapped, or `Bad Address at <address>` is raised.

---

### How to read it

- Loads 2 bytes, **little-endian** (`low` at `address`, `high` at `address+1`).
- Unaligned reads are allowed.

---

### Examples
```basic
REM Read a 16-bit word
W = PEEKW(&100020)
PRINT W
```

---

### Notes
- Match the width to the device register size for MMIO.

**See also:**  
\ref PEEK "PEEK" · \ref PEEKD "PEEKD" · \ref PEEKQ "PEEKQ" · \ref POKEW "POKEW"