\page ASC ASC Function

```basic
ASC(string-expression)
```

Returns the **ASCII code** of the first character in `string-expression`.

---

### Examples

```basic
PRINT ASC("A")
```

This example produces `65`.

```basic
C$ = "Retro Rocket"
PRINT ASC(C$)
```

This example produces `82`, the ASCII code for `"R"`.

```basic
REM Using ASC to check a keypress
key$ = INKEY$
IF key$ > "" THEN
    code = ASC(key$)
    PRINT "You pressed: "; code
ENDIF
```

---

### Notes

* Only the **first character** of the string is considered.
* If the string is **empty**, `ASC` returns `0`.
* Values returned range from **0–255**.

---

**See also:**
\ref CHR "CHR$" · \ref INKEY "INKEY$"
