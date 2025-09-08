\page OUTPORTW OUTPORTW Keyword
```basic
OUTPORTW port, value
```

Writes the low **16 bits** of `value` to `port`.

---

### How to read it

- Writes two bytes as a word to the port.
- Used for devices like ATA data registers.

---

### Examples
```basic
REM Write a word to ATA data port
OUTPORTW &1F0, &1234
```

---

### Notes
- Only use when the port supports 16-bit writes.

**See also:**  
\ref OUTPORT "OUTPORT" · \ref OUTPORTD "OUTPORTD" · \ref OUTPORTQ "OUTPORTQ" · \ref INPORTW "INPORTW"