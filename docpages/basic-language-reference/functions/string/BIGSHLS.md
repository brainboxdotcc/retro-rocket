\page BIGSHLS BIGSHL$ Function

```basic
BIGSHL$(string-expression, integer-expression)
```

Shifts an **arbitrary precision integer** (as a decimal string) **left by N bits**, returning the result as a string.

---

### Examples

```basic
PRINT BIGSHL$("1", 1)
```

Produces `"2"`.

```basic
PRINT BIGSHL$("3", 2)
```

Produces `"12"`.

```basic
PRINT BIGSHL$("-5", 3)
```

Produces `"-40"`.

---

### Notes

* Input must be a valid base-10 integer string.
* The shift count must be a non-negative integer.
* A left shift by `N` bits is equivalent to multiplying by `2^N`.
* Negative numbers are supported.
* Result is returned as a decimal string.

---

**See also:**
\ref BIGSHRS "BIGSHR$" · \ref BIGMULS "BIGMUL$" · \ref BIGADDS "BIGADD$"
