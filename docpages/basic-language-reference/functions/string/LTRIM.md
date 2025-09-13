\page LTRIM LTRIM$ Function

```basic
LTRIM$(string-expression)
```

Removes all leading spaces from the start of the string expression and returns the result.

---

### Examples

```basic
PRINT LTRIM$("   Hello")
```

Produces `"Hello"`.

```basic
PRINT LTRIM$("   Retro Rocket   ")
```

Produces `"Retro Rocket   "` (only leading spaces removed).

```basic
REM Trim input before processing
input$ = "   command"
PRINT "Command = ["; LTRIM$(input$); "]"
```

---

### Notes

* Only space characters (`CHR$(32)`) are removed; tabs and other whitespace are not affected.
* If the string has no leading spaces, it is returned unchanged.
* Use \ref RTRIM "RTRIM$" to remove trailing spaces, or \ref TRIM "TRIM$" to remove both ends.

---

**See also:**
\ref RTRIM "RTRIM$" · \ref TRIM "TRIM$" · \ref LEN "LEN"
