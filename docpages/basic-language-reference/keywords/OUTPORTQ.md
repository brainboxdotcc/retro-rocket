\page OUTPORTQ OUTPORTQ Keyword
```basic
OUTPORTQ port, value
```

Writes a **64-bit quad-word** to `port`.


> Very few devices support 64-bit port writes; consult hardware documentation before use.

---

### How to read it

- Writes eight bytes as a 64-bit value to the port.

---

### Examples
```basic
REM Example 64-bit port write
OUTPORTQ &1234, &0123456789ABCDEF
```

---

### Notes
- Rare in practice; most hardware uses MMIO or smaller port widths.

**See also:**  
[OUTPORT](https://github.com/brainboxdotcc/retro-rocket/wiki/OUTPORT) · [OUTPORTW](https://github.com/brainboxdotcc/retro-rocket/wiki/OUTPORTW) · [OUTPORTD](https://github.com/brainboxdotcc/retro-rocket/wiki/OUTPORTD) · [INPORTQ](https://github.com/brainboxdotcc/retro-rocket/wiki/INPORTQ)