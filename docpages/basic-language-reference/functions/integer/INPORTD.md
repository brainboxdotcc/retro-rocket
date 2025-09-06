\page INPORTD INPORTD Function
```basic
X = INPORTD(port)
```

Reads a **32-bit double-word** from `port` and returns it.


> Only use on devices that specify **double-word** port access.

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
[INPORT](https://github.com/brainboxdotcc/retro-rocket/wiki/INPORT) · [INPORTW](https://github.com/brainboxdotcc/retro-rocket/wiki/INPORTW) · [INPORTQ](https://github.com/brainboxdotcc/retro-rocket/wiki/INPORTQ) · [OUTPORTD](https://github.com/brainboxdotcc/retro-rocket/wiki/OUTPORTD)