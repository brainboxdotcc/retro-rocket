\page RTRIM RTRIM$ Function

```basic
RTRIM$(string-expression)
```

Removes all trailing spaces from the end of the string expression and returns the result.

---

### Examples

```basic
PRINT RTRIM$("Hello   ")
```

Produces `"Hello"`.

```basic
PRINT RTRIM$("Retro Rocket   ")
```

Produces `"Retro Rocket"` (only trailing spaces removed).

```basic
REM Trim before concatenating
a$ = "Name   "
b$ = "Surname"
PRINT RTRIM$(a$) + " " + b$
```

---

### Notes

* Only space characters (`CHR$(32)`) are removed; tabs and other whitespace are not affected.
* If the string has no trailing spaces, it is returned unchanged.
* Use \ref LTRIM "LTRIM\$" to remove leading spaces, or \ref TRIM "TRIM\$" to remove both ends.

---

**See also:**
\ref LTRIM "LTRIM$" · \ref TRIM "TRIM$" · \ref LEN "LEN"
