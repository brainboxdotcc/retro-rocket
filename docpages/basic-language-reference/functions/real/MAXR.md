\page MAXR MAXR Function

```basic
MAXR(a, b)
```

Returns the **larger** of two real values.

---

### Examples

```basic
REM Simple comparison
PRINT MAXR(3.5, 7.2)
```

```basic
REM Clamp a value to a minimum
VALUE# = -1.25
VALUE# = MAXR(VALUE#, 0.0)
PRINT VALUE#
```

```basic
REM Use in an expression
A# = 2.75
B# = 2.80
PRINT "Largest is "; MAXR(A#, B#)
```

---

### Notes

* Both parameters must be **real expressions**.
* Returns whichever value is higher.
* Commonly used for clamping values or enforcing lower bounds with real numbers.
* For integer values, use \ref MAX "MAX".

---

**See also:**
\ref MAX "MAX" · \ref MIN "MIN" · \ref ABS "ABS"
