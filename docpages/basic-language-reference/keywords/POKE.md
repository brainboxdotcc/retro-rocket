\page POKE POKE Keyword
```basic
POKE address, value
```

Stores the low **8 bits** of `value` to memory at `address`.


\remark The `address` must be writable. If not, `Bad Address at <address>` is raised.
\remark Writing to MMIO can have immediate hardware effects.

---

### How to read it

- Think **“store 8-bit”**.
- Only the lowest byte of `value` is written.

---

### Examples
```basic
REM Store a marker byte
POKE &100100, &AA
PRINT PEEK(&100100)
```

---

### Notes
- Always use the access width expected by the device.

**See also:**  
[POKEW](https://github.com/brainboxdotcc/retro-rocket/wiki/POKEW) · [POKED](https://github.com/brainboxdotcc/retro-rocket/wiki/POKED) · [POKEQ](https://github.com/brainboxdotcc/retro-rocket/wiki/POKEQ) · [PEEK](https://github.com/brainboxdotcc/retro-rocket/wiki/PEEK)