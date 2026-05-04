\page BIGDIVS BIGDIV$ Function

```basic
BIGDIV$(string-expression, string-expression)
```

Divides one **arbitrary precision integer** (as a decimal string) by another, returning the **quotient** as a string.

---

### Examples

```basic
PRINT BIGDIV$("100000000000000000000", "10")
```

Produces `"10000000000000000000"`.

```basic
PRINT BIGDIV$("7", "2")
```

Produces `"3"`.

```basic
PRINT BIGDIV$("-100", "4")
```

Produces `"-25"`.

---

### Notes

* Inputs must be valid base-10 integer strings.
* Division is integer division (fractional part is discarded).
* Division by zero produces an error.
* Negative numbers are supported.
* Result is returned as a decimal string.

---

**See also:**
\ref BIGMODS "BIGMOD$" · \ref BIGADDS "BIGADD$" · \ref BIGSUBS "BIGSUB$" · \ref BIGMULS "BIGMUL$" · \ref BIGCMP "BIGCMP"
