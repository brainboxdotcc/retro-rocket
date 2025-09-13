\page LJUST LJUST$ Function

```basic
LJUST$(string-expression, integer-expression, string-expression)
```

Returns a **left-justified string** by padding the input string (first parameter) with the pad string (third parameter) until the result length is **at least** the specified width (second parameter).

---

### Examples

```basic
PRINT LJUST$("Retro", 10, " ")
```

Produces `"Retro     "` (padded with spaces).

```basic
PRINT LJUST$("42", 5, "0")
```

Produces `"42000"`.

```basic
REM Use multi-character padding
PRINT LJUST$("X", 6, "-=")
```

Produces `"X-=-=-"`.

---

### Notes

* If the original string is already at least the specified length, it is returned unchanged.
* The pad string is repeated as many times as necessary, and may be truncated to fit exactly.
* Pad string must not be empty; an empty pad string raises an error.
* Useful for creating fixed-width tables or aligning output.

---

**See also:**
\ref RJUST "RJUST$" · \ref LEN "LEN"
