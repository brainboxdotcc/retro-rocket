\page REVERSE REVERSE$ Function

```basic
REVERSE$(string-expression)
```

Reverses the characters in the given string expression and returns the result.

---

### Examples

| Input            | Output           |
| ---------------- | ---------------- |
| `"abcdef"`       | `"fedcba"`       |
| `"0123456789"`   | `"9876543210"`   |
| `"hello world!"` | `"!dlrow olleh"` |
| `"Retro Rocket"` | `"tekcoR orteR"` |
| `"BASIC"`        | `"CISAB"`        |

```basic
PRINT REVERSE$("Reverse me!")
```

Produces `"!em esreveR"`.

```basic
REM Use REVERSE$ in a palindrome check
word$ = "level"
IF word$ = REVERSE$(word$) THEN
    PRINT word$; " is a palindrome"
ENDIF
```

---

### Notes

* Operates on the string as a sequence of ASCII characters.
* Whitespace, punctuation, and case are preserved exactly.
* Always returns a new string; the original string is unchanged.

---

**See also:**
\ref LEFT "LEFT$" · \ref RIGHT "RIGHT$" · \ref MID "MID$" · \ref LEN "LEN"
