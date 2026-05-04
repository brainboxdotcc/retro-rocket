\page BIGMODS BIGMOD$ Function

```basic
BIGMOD$(string-expression, string-expression)
```

Calculates the **remainder** of dividing one **arbitrary precision integer** (as a decimal string) by another, returning the result as a string.

---

### Examples

```basic
PRINT BIGMOD$("10", "3")
```

Produces `"1"`.

```basic
PRINT BIGMOD$("100000000000000000000", "7")
```

Produces `"2"`.

```basic
PRINT BIGMOD$("-10", "3")
```

Produces `"-1"`.

---

### Notes

* Inputs must be valid base-10 integer strings.
* Division by zero produces an error.
* The result follows the sign of the dividend.
* Result is returned as a decimal string.

---

**See also:**
\ref BIGDIVS "BIGDIV$" · \ref BIGADDS "BIGADD$" · \ref BIGSUBS "BIGSUB$" · \ref BIGMULS "BIGMUL$" · \ref BIGCMP "BIGCMP"
