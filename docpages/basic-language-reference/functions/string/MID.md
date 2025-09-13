\page MID MID$ Function

```basic
MID$(string-expression, integer-expression, integer-expression)
```

Returns a **substring** of the given string, starting at the position given by the first integer parameter and extending for the length given by the second.

---

### Examples

```basic
PRINT MID$("RetroRocket", 0, 5)
```

Produces `"Retro"`.

```basic
PRINT MID$("RetroRocket", 5, 6)
```

Produces `"Rocket"`.

```basic
REM Extract file extension
filename$ = "document.txt"
PRINT MID$(filename$, LEN(filename$) - 3, 3)
```

Produces `"txt"`.

---

### Notes

* Positions are **zero-based**: the first character of the string is at index `0`.
* If the length extends beyond the end of the string, the substring is truncated.
* If the starting index is beyond the string length, the result is an empty string.
* Negative positions or lengths cause an error.

---

**See also:**
\ref LEFT "LEFT$" · \ref RIGHT "RIGHT$" · \ref LEN "LEN"
