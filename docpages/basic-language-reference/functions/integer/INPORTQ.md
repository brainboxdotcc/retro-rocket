\page INPORTQ INPORTQ Function
```basic
X = INPORTQ(port)
```

Reads a **64-bit quad-word** from `port` and returns it.


> Very few devices support 64-bit port access; consult hardware documentation before use.

---

### How to read it

- Reads eight consecutive bytes from the port as a 64-bit value.

---

### Examples
```basic
REM Example 64-bit port read
Q = INPORTQ(&1234)
PRINT Q
```

---

### Notes
- Rare in practice; most hardware uses MMIO or smaller port widths.

**See also:**  
[INPORT](https://github.com/brainboxdotcc/retro-rocket/wiki/INPORT) · [INPORTW](https://github.com/brainboxdotcc/retro-rocket/wiki/INPORTW) · [INPORTD](https://github.com/brainboxdotcc/retro-rocket/wiki/INPORTD) · [OUTPORTQ](https://github.com/brainboxdotcc/retro-rocket/wiki/OUTPORTQ)