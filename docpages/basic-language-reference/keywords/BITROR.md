\page BITROR BITROR Keyword
```basic
X = BITROR(a, n, width)
```

Rotates integer `a` **right** by `n` bits within a given `width`.

---

### How to read it
- Only the lowest `width` bits participate in the rotation.
- Width is typically 1–64.
- Result is masked back to the chosen width.

---

### Examples
```basic
PRINT BITROR(&9, 1, 4)
```

This rotates `1001₂` right by one within 4 bits, giving `1100₂ = 12`.

---

### Notes
- Operates on 64-bit integers.
- Rotation width must be chosen carefully; too small discards low bits.

**See also:**  
\ref BITROL "BITROL" · \ref BITSHR "BITSHR"