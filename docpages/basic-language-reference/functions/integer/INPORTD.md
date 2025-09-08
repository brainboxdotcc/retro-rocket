\page INPORTD INPORTD Function
```basic
X = INPORTD(port)
```

Reads a **32-bit double-word** from `port` and returns it.


@note Only use on devices that specify **double-word** port access.

---

### How to read it

- Reads four consecutive bytes from the port as a 32-bit value.
- Common for PCI configuration space.

---

### Examples
```basic
REM Read PCI config data
D = INPORTD(&CFC)
PRINT D
```

---

### Notes
- Most modern devices prefer MMIO; only use when the hardware requires 32-bit port access.

**See also:**  
\ref INPORT "INPORT" · \ref INPORTW "INPORTW" · \ref INPORTQ "INPORTQ" · \ref OUTPORTD "OUTPORTD"