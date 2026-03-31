\page MIN MIN Function

```basic
MIN(a, b)
```

Returns the **smaller** of two integer values.

---

### Examples

```basic
REM Simple comparison
PRINT MIN(10, 5)
```

```basic
REM Clamp a value to a maximum
VALUE = 120
VALUE = MIN(VALUE, 100)
PRINT VALUE
```

```basic
REM Use in an expression
A = 42
B = 17
PRINT "Smallest is "; MIN(A, B)
```

---

### Notes

* Both parameters must be **integer expressions**.
* Returns whichever value is lower.
* Commonly used for clamping values or enforcing limits.
* For real values, use \ref MINR "MINR"

---

**See also:**
\ref MAX "MAX" · \ref ABS "ABS"
