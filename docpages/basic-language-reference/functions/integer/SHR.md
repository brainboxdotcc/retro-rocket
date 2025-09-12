\page SHR SHR Function

```basic
SHR(value, count)
```

Performs a **bitwise shift right** operation on an integer.
The first parameter is the value to shift, the second is the number of bit positions.

---

### Examples

```basic
PRINT SHR(4, 1)
```

Produces `2` (`0100` → `0010`).

```basic
PRINT SHR(9, 2)
```

Produces `2` (`1001` → `0010`, fractional part discarded).

```basic
REM Divide by powers of two using SHR
n = 40
PRINT SHR(n, 3)   ' Same as 40 / 8 = 5
```

---

### Notes

* Operates on 64-bit integers.
* Shifting right by `n` positions is equivalent to integer division by `2^n`.
* Bits shifted out on the right are discarded; new bits on the left are filled with zero.
* This is a **logical shift** (fills with zero), not an arithmetic shift. Negative values are not sign-extended.
* If the shift count is negative or larger than the bit width, behaviour is undefined.

---

**See also:**
\ref SHL "SHL" · \ref BITAND "BITAND" · \ref BITOR "BITOR" · \ref BITNAND "BITNAND"
