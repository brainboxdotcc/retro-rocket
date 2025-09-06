\page BITXNOR BITXNOR Keyword
```basic
X = BITXNOR(a, b)
```

Computes the **bitwise equivalence** (XNOR) of two integers.

---

### How to read it
- Equivalent to `NOT (a XOR b)`.
- Returns all bits that are the same in both operands.

---

### Examples
```basic
PRINT BITXNOR(&6, &3)
```

---

### Notes
- Operates on 64-bit integers.

**See also:**  
[BITEOR](https://github.com/brainboxdotcc/retro-rocket/wiki/BITEOR) · [BITNOT](https://github.com/brainboxdotcc/retro-rocket/wiki/BITNOT)