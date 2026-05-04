\page BIGCMP BIGCMP Function

```basic
BIGCMP(string-expression, string-expression)
```

Compares two **arbitrary precision integers** provided as **decimal strings**.

---

### Examples

```basic
PRINT BIGCMP("100", "50")
```

Produces `1`.

```basic
PRINT BIGCMP("42", "42")
```

Produces `0`.

```basic
PRINT BIGCMP("-10", "5")
```

Produces `-1`.

---

### Notes

* Inputs must be valid base-10 integer strings.
* Returns:

  * `1` if first value is greater than second
  * `0` if both values are equal
  * `-1` if first value is less than second
* Supports values far beyond 64-bit limits.
* Negative numbers are supported.

---

**See also:**
\ref BIGADDS "BIGADD$" · \ref BIGSUBS "BIGSUB$" · \ref BIGMULS "BIGMUL$" · \ref BIGDIVS "BIGDIV$" · \ref BIGMODS "BIGMOD$"
