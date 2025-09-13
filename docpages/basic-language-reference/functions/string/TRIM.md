\page TRIM TRIM$ Function

```basic
TRIM$(string-expression)
```

Removes all leading and trailing spaces from the string expression and returns the result.

---

### Examples

```basic
PRINT TRIM$("   Hello   ")
```

Produces `"Hello"`.

```basic
REM Normalise user input
input$ = "   command   "
PRINT "["; TRIM$(input$); "]"
```

Produces `[command]`.

---

### Notes

* Only space characters (`CHR$(32)`) are removed; tabs and other whitespace are not affected.
* If the string has no leading or trailing spaces, it is returned unchanged.
* For trimming only one side: use \ref LTRIM "LTRIM\$" or \ref RTRIM "RTRIM\$".

---

**See also:**
\ref LTRIM "LTRIM$" · \ref RTRIM "RTRIM$" · \ref LEN "LEN"
