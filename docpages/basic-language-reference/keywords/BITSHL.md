\page BITSHL BITSHL Keyword
```basic
X = BITSHL(a, n)
```

Shifts integer `a` **left** by `n` bits.

---

### How to read it
- Equivalent to multiplying `a` by 2ⁿ.
- Low-order bits are filled with zeros.
- Shift counts outside 0–63 are clamped.

---

### Examples
```basic
PRINT BITSHL(3, 2)
```

This example produces `12`.

---

### Notes
- Operates on 64-bit integers.

**See also:**  
[BITSHR](https://github.com/brainboxdotcc/retro-rocket/wiki/BITSHR) · [BITROL](https://github.com/brainboxdotcc/retro-rocket/wiki/BITROL)