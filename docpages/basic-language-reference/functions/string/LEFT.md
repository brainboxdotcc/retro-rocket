\page LEFT LEFT$ Function

```basic
LEFT$(string-expression, integer-expression)
```

Returns the **leftmost characters** from a string.
The second parameter specifies how many characters to return.

---

### Examples

```basic
PRINT LEFT$("RetroRocket", 5)
```

Produces `Retro`.

```basic
REM Extract drive letter from a path
path$ = "C:/users/alice"
PRINT LEFT$(path$, 2)
```

Produces `C:`.

```basic
REM Safely clip a string
msg$ = "Hello, world!"
PRINT LEFT$(msg$, 80)
```

Prints the message, clipped to 80 characters if longer.

---

### Notes

* If the length requested is greater than the string length, the entire string is returned.
* If the length is `0`, an empty string is returned.
* Negative lengths cause an error.
* Result is always a new string, the original is unchanged.

---

**See also:**
\ref MID "MID$" · \ref RIGHT "RIGHT$" · \ref LEN "LEN"
