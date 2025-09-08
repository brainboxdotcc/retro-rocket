\page BITROL BITROL Keyword
```basic
X = BITROL(a, n, width)
```

Rotates integer `a` **left** by `n` bits within a given `width`.

---

### How to read it
- Only the lowest `width` bits participate in the rotation.
- Width is typically 1–64.
- Result is masked back to the chosen width.

---

### Examples
```basic
PRINT BITROL(&9, 1, 4)
```

This rotates `1001₂` left by one within 4 bits, giving `0011₂ = 3`.

---

### Notes
- Operates on 64-bit integers.
- Rotation width must be chosen carefully; too small discards high bits.

**See also:**  
\ref BITROR "BITROR" · \ref BITSHL "BITSHL"