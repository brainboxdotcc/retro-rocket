\page SHL SHL Function

```basic
SHL(value, count)
```

Performs a **bitwise shift left** operation on an integer.
The first parameter is the value to shift, the second is the number of bit positions.

---

### Examples

```basic
PRINT SHL(1, 1)
```

Produces `2` (`0001` → `0010`).

```basic
PRINT SHL(3, 2)
```

Produces `12` (`0011` → `1100`).

```basic
REM Multiply by powers of two using SHL
n = 7
PRINT SHL(n, 3)   ' Same as 7 * 8 = 56
```

---

### Notes

* Operates on 64-bit integers.
* Shifting left by `n` positions is equivalent to multiplying by `2^n`.
* Bits shifted out on the left are discarded; new bits on the right are filled with zero.
* If the shift count is negative or larger than the bit width, behaviour is undefined.

---

**See also:**
\ref SHR "SHR" · \ref BITAND "BITAND" · \ref BITOR "BITOR" · \ref BITNAND "BITNAND"
