\page BIGMULS BIGMUL$ Function

```basic
BIGMUL$(string-expression, string-expression)
```

Multiplies two **arbitrary precision integers** provided as **decimal strings**, returning the result as a string.

---

### Examples

```basic
PRINT BIGMUL$("123456789", "10")
```

Produces `"1234567890"`.

```basic
PRINT BIGMUL$("999999999999999999", "999999999999999999")
```

Produces `"999999999999999998000000000000000001"`.

```basic
PRINT BIGMUL$("-25", "4")
```

Produces `"-100"`.

---

### Notes

* Inputs must be valid base-10 integer strings.
* Supports values far beyond 64-bit limits.
* Negative numbers are supported.
* Result is returned as a decimal string.
* Leading zeros in input are ignored.

---

**See also:**
\ref BIGADDS "BIGADD$" · \ref BIGSUBS "BIGSUB$" · \ref BIGDIVS "BIGDIV$" · \ref BIGMODS "BIGMOD$" · \ref BIGCMP "BIGCMP"
