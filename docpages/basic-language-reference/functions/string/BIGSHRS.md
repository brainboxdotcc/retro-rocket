\page BIGSHRS BIGSHR$ Function

```basic
BIGSHR$(string-expression, integer-expression)
```

Shifts an **arbitrary precision integer** (as a decimal string) **right by N bits**, returning the result as a string.

---

### Examples

```basic
PRINT BIGSHR$("8", 1)
```

Produces `"4"`.

```basic
PRINT BIGSHR$("15", 2)
```

Produces `"3"`.

```basic
PRINT BIGSHR$("-16", 2)
```

Produces `"-4"`.

---

### Notes

* Input must be a valid base-10 integer string.
* The shift count must be a non-negative integer.
* A right shift by `N` bits is equivalent to integer division by `2^N`.
* Fractional parts are discarded.
* Negative numbers are supported.
* Result is returned as a decimal string.

---

**See also:**
\ref BIGSHLS "BIGSHL$" · \ref BIGDIVS "BIGDIV$" · \ref BIGADDS "BIGADD$"
