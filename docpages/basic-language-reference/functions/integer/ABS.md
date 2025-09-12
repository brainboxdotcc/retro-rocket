\page ABS ABS Function

```basic
ABS(numeric-expression)
```

Returns the **absolute value** of a numeric expression.
Negative values are converted to positive; positive values remain unchanged.

---

### How to read it

* For integers: `ABS(-5)` becomes `5`.
* For reals: `ABS(-3.14)` becomes `3.14`.
* Zero always remains `0`.

---

### Examples

```basic
PRINT ABS(-42)
```

This example produces `42`.

```basic
A# = -12.5
PRINT ABS(A#)
```

This example produces `12.5`.

```basic
REM Using ABS in a loop to ensure non-negative distances
FOR x = -5 TO 5
    PRINT "Distance: "; ABS(x)
NEXT
```

---

### Notes

* Works with both **integers** and **reals**.
* Has no effect on `0`.
* Useful in algorithms where only magnitudes matter (e.g. distances, error values).

---

**See also:**
\ref SGN "SGN" · \ref INT "INT" · \ref SQR "SQR"
