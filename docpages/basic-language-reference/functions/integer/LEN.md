\page LEN LEN Function

```basic
LEN(string-expression)
```

Returns the **length** of a string in **ASCII characters**.
The count includes all characters in the string, starting at `1` for the first character.

---

### Examples

```basic
PRINT LEN("Retro Rocket")
```

Produces `12`.

```basic
REM Length of an empty string
PRINT LEN("")
```

Produces `0`.

```basic
REM Use LEN to iterate over characters
name$ = "HELLO"
FOR i = 1 TO LEN(name$)
    PRINT MID$(name$, i - 1, 1)
NEXT
```

```basic
REM Truncate a string if too long
line$ = "This is a long string"
IF LEN(line$) > 10 THEN
    PRINT "Truncated: "; LEFT$(line$, 10)
ENDIF
```

---

### Notes

* Returns an integer value ≥ 0.
* Length is measured in characters, not bytes — since Retro Rocket BASIC strings are ASCII, the two are identical.
* Useful together with \ref LEFT "LEFT$", \ref RIGHT "RIGHT$", and \ref MID "MID$" for string slicing.

---

**See also:**
\ref LEFT "LEFT\$" · \ref RIGHT "RIGHT\$" · \ref MID "MID\$" · \ref ASC "ASC"
