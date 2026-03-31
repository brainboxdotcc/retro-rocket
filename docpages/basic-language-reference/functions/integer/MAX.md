\page MAX MAX Function

```basic
MAX(a, b)
```

Returns the **larger** of two integer values.

---

### Examples

```basic
REM Simple comparison
PRINT MAX(10, 5)
```

```basic
REM Clamp a value to a minimum
VALUE = -10
VALUE = MAX(VALUE, 0)
PRINT VALUE
```

```basic
REM Use in an expression
A = 42
B = 17
PRINT "Largest is "; MAX(A, B)
```

---

### Notes

* Both parameters must be **integer expressions**.
* Returns whichever value is higher.
* Commonly used for clamping values or enforcing lower bounds.
* For real values, use \ref MAXR "MAXR".

---

**See also:**
\ref MIN "MIN" · \ref ABS "ABS"
