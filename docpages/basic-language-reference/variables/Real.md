\page type-real Real Variables

Real variables are represented by any variable with the suffix `#`, e.g. `B#` or `MYVAR#`.
They are stored as **64-bit double-precision IEEE 754 floating point values**.

---

### Examples

```basic
PI# = 3.14159
radius# = 5.5
area# = PI# * radius# * radius#
PRINT "Area = "; area#
```

```basic
REM Mixing integers and reals
A = 10
B# = 3
PRINT A / B#
```

Produces `3.33333...` since the operation is promoted to real.

---

### Notes

* Range and precision follow IEEE 754 double format.
* Supports fractional values, scientific notation, and very large or very small numbers.
* Operations involving both integer and real variables are promoted to real.
* Rounding can be applied with \ref ROUND "ROUND".

---

**See also:**
\ref type-integer "Integer Variables" · \ref type-string "String Variables" · \ref type-array "Array Variables"
