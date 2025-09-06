\page PEEKD PEEKD Function
```basic
X = PEEKD(address)
```

Reads a **32-bit double-word** from `address` and returns it as an integer (0–4294967295).


> The entire 4-byte span must be valid; otherwise `Bad Address at <address>` is raised.

---

### How to read it

- Loads 4 bytes in **little-endian** order.
- Unaligned reads are supported.

---

### Examples
```basic
REM Read a 32-bit value
D = PEEKD(&100040)
PRINT D
```

---

### Notes
- 32-bit is common for MMIO registers.

**See also:**  
[PEEK](https://github.com/brainboxdotcc/retro-rocket/wiki/PEEK) · [PEEKW](https://github.com/brainboxdotcc/retro-rocket/wiki/PEEKW) · [PEEKQ](https://github.com/brainboxdotcc/retro-rocket/wiki/PEEKQ) · [POKED](https://github.com/brainboxdotcc/retro-rocket/wiki/POKED)