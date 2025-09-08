\page PEEK PEEK Function
```basic
X = PEEK(address)
```

Reads a **single byte** from the specified memory `address` and returns it as an integer (0–255).


@note `PEEK` reads only from **mapped** memory. If `address` (or any byte it touches) is not permitted by the system memory map, an error is raised: `Bad Address at <address>`.

---

### How to read it

- Think of `PEEK(addr)` as **“load 8-bit from RAM/MMIO at `addr`”**.
- Unaligned addresses are fine on x86-64.
- Values are read **little-endian**.

---

### Examples
```basic
REM Read three consecutive bytes
BASE = &100000
PRINT PEEK(BASE)
PRINT PEEK(BASE+1)
PRINT PEEK(BASE+2)
```

```basic
REM Read a status byte
STATUS = PEEK(&200000)
IF (STATUS AND 1) <> 0 THEN PRINT "Ready"
```

---

### Notes
- The accessed range must be valid according to the boot-time memory map.
- Accessing device MMIO may have **side effects**.

**See also:**  
\ref PEEKW "PEEKW" · \ref PEEKD "PEEKD" · \ref PEEKQ "PEEKQ" · \ref POKE "POKE"