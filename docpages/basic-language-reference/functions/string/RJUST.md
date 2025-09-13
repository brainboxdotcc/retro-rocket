\page RJUST RJUST$ Function

```basic
RJUST$(string-expression, integer-expression, string-expression)
```

Returns a **right-justified string** by padding the input string (first parameter) with the pad string (third parameter) on the **left**, until the result length is **at least** the specified width (second parameter).

---

### Examples

```basic
PRINT RJUST$("Retro", 10, " ")
```

Produces `"     Retro"`.

```basic
PRINT RJUST$("42", 5, "0")
```

Produces `"00042"`.

```basic
REM Use multi-character padding
PRINT RJUST$("X", 6, "-=")
```

Produces `"-=-=-X"`.

---

### Notes

* If the original string is already at least the specified length, it is returned unchanged.
* The pad string is repeated as many times as necessary, and truncated if required to fit exactly.
* Pad string must not be empty; an empty pad string raises an error.
* Useful for formatting tables, zero-padding numbers, and aligning output.

---

**See also:**
\ref LJUST "LJUST\$" · \ref LEN "LEN"
