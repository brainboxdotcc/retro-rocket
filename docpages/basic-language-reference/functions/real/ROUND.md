\page ROUND ROUND Function

```basic
ROUND(real-expression)
```

Rounds the provided real value to the **nearest integer**.
Values with a fractional part of `0.5` or greater are rounded **away from zero**.

---

### Examples

```basic
PRINT ROUND(3.2)
```

Produces `3`.

```basic
PRINT ROUND(3.5)
```

Produces `4`.

```basic
PRINT ROUND(-2.5)
```

Produces `-3`.

```basic
REM Compare ROUND with INT (truncate) and CEIL (round up)
x# = -1.7
PRINT "ROUND("; x#; ") = "; ROUND(x#)   ' -2
PRINT "INT("; x#; ")   = "; INT(x#)     ' -1 (towards zero)
PRINT "CEIL("; x#; ")  = "; CEIL(x#)    ' -1 (towards +infinity)
```

---

### Notes

* Returns a 64-bit integer.
* Halfway cases (`.5`) always round **away from zero**:

  * `ROUND(2.5)` → `3`
  * `ROUND(-2.5)` → `-3`
* Differs from \ref INT "INT", which truncates towards zero, and from \ref CEIL "CEIL", which always rounds up.

---

**See also:**
\ref INT "INT" · \ref CEIL "CEIL"
