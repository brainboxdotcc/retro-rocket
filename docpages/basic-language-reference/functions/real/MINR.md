\page MINR MINR Function

```basic
MINR(a, b)
```

Returns the **smaller** of two real values.

---

### Examples

```basic
REM Simple comparison
PRINT MINR(3.5, 7.2)
```

```basic
REM Clamp a value to a maximum
VALUE# = 5.75
VALUE# = MINR(VALUE#, 3.0)
PRINT VALUE#
```

```basic
REM Use in an expression
A# = 2.75
B# = 2.80
PRINT "Smallest is "; MINR(A#, B#)
```

---

### Notes

* Both parameters must be **real expressions**.
* Returns whichever value is lower.
* Commonly used for clamping values or enforcing upper bounds with real numbers.
* For integer values, use \ref MIN "MIN".

---

**See also:**
\ref MIN "MIN" · \ref MAXR "MAXR" · \ref ABS "ABS"
