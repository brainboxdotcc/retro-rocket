\page RADIXS RADIX$ Function

```basic
RADIX$(integer-expression, integer-expression)
```

Converts an **integer value** (first parameter) into a **string representation** using the radix (base) specified by the second parameter.
The radix must be between `2` and `36`.

---

### Examples

```basic
PRINT RADIX$(255, 16)
```

Produces `"FF"`.

```basic
PRINT RADIX$(10, 2)
```

Produces `"1010"`.

```basic
PRINT RADIX$(12345, 36)
```

Produces `"9IX"`.

---

### Notes

* Valid radix range is `2` (binary) through `36` (digits + A–Z).
* Returned value is always uppercase.
* Negative numbers are prefixed with `-`.
* Useful for formatting numbers in arbitrary bases beyond standard decimal, hexadecimal, or octal.
* To parse a string in an arbitrary base back into an integer, use \ref RADIX "RADIX".

---

**See also:**
\ref RADIX "RADIX" · \ref HEXVAL "HEXVAL" · \ref OCTVAL "OCTVAL" · \ref VAL "VAL"
