\page BITNOT BITNOT Keyword
```basic
X = BITNOT(a)
```

Computes the **bitwise complement** of an integer.

---

### How to read it
- Flips all bits: ones become zeros, zeros become ones.
- Uses two’s-complement semantics on 64-bit integers.

---

### Examples
```basic
PRINT BITNOT(0)
PRINT BITNOT(&FF)
```

---

### Notes
- Output may appear as a large negative number, due to two’s-complement representation.

**See also:**  
[BITEOR](https://github.com/brainboxdotcc/retro-rocket/wiki/BITEOR) · [BITNOR](https://github.com/brainboxdotcc/retro-rocket/wiki/BITNOR) · [BITXNOR](https://github.com/brainboxdotcc/retro-rocket/wiki/BITXNOR)