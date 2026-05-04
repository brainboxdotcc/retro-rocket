\page BIGGCDS BIGGCD$ Function

```basic
BIGGCD$(string-expression, string-expression)
```

Calculates the **greatest common divisor (GCD)** of two **arbitrary precision integers** provided as decimal strings, returning the result as a string.

---

### Examples

```basic
PRINT BIGGCD$("48", "18")
```

Produces `"6"`.

```basic
PRINT BIGGCD$("12345678901234567890", "9876543210")
```

Produces `"90"`.

```basic
PRINT BIGGCD$("-42", "56")
```

Produces `"14"`.

---

### Notes

* Inputs must be valid base-10 integer strings.
* Negative values are treated by magnitude (the result is always non-negative).
* If both inputs are zero, the result is `"0"`.
* Result is returned as a decimal string.

---

**See also:**
\ref BIGADDS "BIGADD$" · \ref BIGSUBS "BIGSUB$" · \ref BIGMULS "BIGMUL$" · \ref BIGDIVS "BIGDIV$" · \ref BIGMODS "BIGMOD$" · \ref BIGCMP "BIGCMP"
