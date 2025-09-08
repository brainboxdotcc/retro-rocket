\page OUTPORTD OUTPORTD Keyword
```basic
OUTPORTD port, value
```

Writes the low **32 bits** of `value` to `port`.

---

### How to read it

- Writes four bytes as a 32-bit value to the port.
- Used for PCI configuration space.

---

### Examples
```basic
REM Write PCI config address
OUTPORTD &CF8, &80000000
```

---

### Notes
- Only use where the hardware expects a 32-bit port write.

**See also:**  
\ref OUTPORT "OUTPORT" · \ref OUTPORTW "OUTPORTW" · \ref OUTPORTQ "OUTPORTQ" · \ref INPORTD "INPORTD"