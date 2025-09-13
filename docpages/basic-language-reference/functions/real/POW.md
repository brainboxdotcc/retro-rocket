\page POW POW Function

```basic
POW(real-expression, real-expression)
```

Raises the **first expression** to the power of the **second expression** and returns the result as a real number.

---

### Examples

```basic
PRINT POW(2, 3)
```

Produces `8`.

```basic
PRINT POW(9, 0.5)
```

Produces `3` (square root of 9).

```basic
PRINT POW(16, -1)
```

Produces `0.0625` (reciprocal of 16).

```basic
REM Compute compound interest
principal# = 1000
rate# = 0.05
years = 10
amount# = principal# * POW(1 + rate#, years)
PRINT "Future value = "; amount#
```

---

### Notes

* The base (first parameter) may be any real number.
* The exponent (second parameter) may also be any real, including fractions and negatives.
* Special cases:

  * `POW(x, 0)` returns `1` (for any x ≠ 0).
  * `POW(0, y)` returns `0` (for y > 0).
  * Invalid combinations (e.g. `POW(0, 0)` or negative base with fractional exponent) may cause errors.
* Related: \ref SQR "SQR" computes the square root directly.

---

**See also:**
\ref EXP "EXP" · \ref LOG "LOG" · \ref SQR "SQR"
