\page BITEOR BITEOR Keyword
```basic
X = BITEOR(a, b)
```

Computes the **bitwise exclusive OR (XOR)** of two integers.  
In BBC BASIC this operator was named `EOR`.

---

### How to read it
- Returns bits set when exactly one of `a` or `b` has the bit set.

---

### Examples
```basic
PRINT BITEOR(&6, &3)
```

This example produces `5`.

---

### Notes
- Operates on 64-bit integers.

**See also:**  
\ref BITOR "BITOR" · \ref BITAND "BITAND" · \ref BITXNOR "BITXNOR"