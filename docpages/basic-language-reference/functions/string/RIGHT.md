\page RIGHT RIGHT$ Function

```basic
RIGHT$(string-expression, integer-expression)
```

Returns the **rightmost characters** from a string.
The second parameter specifies how many characters to return.

---

### Examples

```basic
PRINT RIGHT$("RetroRocket", 6)
```

Produces `"Rocket"`.

```basic
REM Extract file extension
filename$ = "document.txt"
PRINT RIGHT$(filename$, 3)
```

Produces `"txt"`.

```basic
REM Clip to last 10 characters
id$ = "USER-2025-SESSION-ABC123"
PRINT RIGHT$(id$, 10)
```

Produces `"SESSION-ABC123"` (truncated to fit length).

---

### Notes

* If the requested length is greater than the string length, the entire string is returned.
* If the length is `0`, an empty string is returned.
* Negative lengths cause an error.
* Result is always a new string, leaving the original unchanged.

---

**See also:**
\ref LEFT "LEFT$" · \ref MID "MID$" · \ref LEN "LEN"
