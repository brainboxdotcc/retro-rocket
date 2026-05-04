\page BIGABSS BIGABS$ Function

```basic
BIGABS$(string-expression)
```

Returns the **absolute value** of an **arbitrary precision integer** provided as a decimal string.

---

### Examples

```basic
PRINT BIGABS$("-12345678901234567890")
```

Produces `"12345678901234567890"`.

```basic
PRINT BIGABS$("42")
```

Produces `"42"`.

```basic
PRINT BIGABS$("0")
```

Produces `"0"`.

---

### Notes

* Input must be a valid base-10 integer string.
* Negative values are converted to positive.
* Zero and positive values are returned unchanged.
* Result is returned as a decimal string.

---

**See also:**
\ref BIGNEGS "BIGNEG$" · \ref BIGADDS "BIGADD$" · \ref BIGSUBS "BIGSUB$" · \ref BIGCMP "BIGCMP"
