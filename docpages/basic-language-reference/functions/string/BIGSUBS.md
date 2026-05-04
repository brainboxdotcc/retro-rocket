\page BIGSUBS BIGSUB$ Function

```basic
BIGSUB$(string-expression, string-expression)
```

Subtracts the second **arbitrary precision integer** (as a decimal string) from the first, returning the result as a string.

---

### Examples

```basic
PRINT BIGSUB$("100000000000000000000", "1")
```

Produces `"99999999999999999999"`.

```basic
PRINT BIGSUB$("50", "75")
```

Produces `"-25"`.

```basic
PRINT BIGSUB$("-10", "-5")
```

Produces `"-5"`.

---

### Notes

* Inputs must be valid base-10 integer strings.
* Supports values far beyond 64-bit limits.
* Negative numbers are supported.
* Result is returned as a decimal string.
* Leading zeros in input are ignored.

---

**See also:**
\ref BIGADDS "BIGADD$" · \ref BIGMULS "BIGMUL$" · \ref BIGDIVS "BIGDIV$" · \ref BIGMODS "BIGMOD$" · \ref BIGCMP "BIGCMP"
