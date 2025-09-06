\page POKEQ POKEQ Keyword
```basic
POKEQ address, value
```

Stores a **64-bit quad-word** at `address` in **little-endian** order.


\remark The 8-byte span must be writable or `Bad Address at <address>` is raised.

---

### How to read it

- Use for 64-bit pointers, descriptors, or counters.

---

### Examples
```basic
REM Store a 64-bit value
POKEQ &100400, &0123456789ABCDEF
PRINT PEEKQ(&100400)
```

---

### Notes
- Unaligned 64-bit writes may tear if observed concurrently.

**See also:**  
[PEEKQ](https://github.com/brainboxdotcc/retro-rocket/wiki/PEEKQ) · [POKED](https://github.com/brainboxdotcc/retro-rocket/wiki/POKED)