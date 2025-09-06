\page INPORTW INPORTW Function
```basic
X = INPORTW(port)
```

Reads a **16-bit word** from `port` and returns it (0–65535).


> Only use when the device expects **word** access.

---

### How to read it

- Reads two consecutive bytes from the port as a word.
- Common for storage controllers such as ATA.

---

### Examples
```basic
REM Read a word from ATA data port
W = INPORTW(&1F0)
PRINT W
```

---

### Notes
- Do not use unless the port is documented as word-accessible.

**See also:**  
[INPORT](https://github.com/brainboxdotcc/retro-rocket/wiki/INPORT) · [INPORTD](https://github.com/brainboxdotcc/retro-rocket/wiki/INPORTD) · [INPORTQ](https://github.com/brainboxdotcc/retro-rocket/wiki/INPORTQ) · [OUTPORTW](https://github.com/brainboxdotcc/retro-rocket/wiki/OUTPORTW)