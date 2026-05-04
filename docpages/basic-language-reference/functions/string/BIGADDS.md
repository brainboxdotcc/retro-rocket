\page BIGADDS BIGADD$ Function

```basic
BIGADD$(string-expression, string-expression)
```

Adds two **arbitrary precision integers** provided as **decimal strings**, returning the result as a string.

---

### Examples

```basic
PRINT BIGADD$("12345678901234567890", "1")
```

Produces `"12345678901234567891"`.

```basic
PRINT BIGADD$("99999999999999999999", "1")
```

Produces `"100000000000000000000"`.

```basic
PRINT BIGADD$("-50", "25")
```

Produces `"-25"`.

---

### Notes

* Inputs must be valid base-10 integer strings.
* Supports values far beyond 64-bit limits.
* Negative numbers are supported.
* Result is returned as a decimal string.
* Leading zeros in input are ignored.

---

**See also:**
\ref BIGSUBS "BIGSUB$" · \ref BIGMULS "BIGMUL$" · \ref BIGDIVS "BIGDIV$" · \ref BIGMODS "BIGMOD$" · \ref BIGCMP "BIGCMP"
