\page STR STR$ Function

```basic
string-value = STR$(numeric-value)
```

Converts a **numeric value** (integer or real) into its **string representation**.

---

### Examples

```basic
PRINT STR$(123)
```

Produces `"123"`.

```basic
PRINT STR$(-45.67)
```

Produces `"-45.67"`.

```basic
REM Concatenate number into a message
score = 9001
PRINT "Score = " + STR$(score)
```

Produces `"Score = 9001"`.

---

### Notes

* Accepts both integers and real numbers.
* Result is a string in plain decimal notation (no prefixes).
* Formatting (e.g. fixed decimal places) is not provided; use string operations if specific formatting is required.

---

**See also:**
\ref VAL "VAL" · \ref REALVAL "REALVAL" · \ref HEXVAL "HEXVAL" · \ref OCTVAL "OCTVAL"
