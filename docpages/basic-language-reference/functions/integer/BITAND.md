\page BITAND BITAND Function
```basic
X = BITAND(a, b)
```

Computes the **bitwise AND** of two integers.

---

### How to read it
- Returns only bits that are set in both `a` and `b`.

---

### Examples
```basic
PRINT BITAND(&6, &3)
```

This example produces `2`.

---

### Notes
- Operates on 64-bit integers.

**See also:**  
\ref BITOR "BITOR" · \ref BITNAND "BITNAND"