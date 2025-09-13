\page UPPER UPPER$ Function

```basic
UPPER$(string-expression)
```

Converts a string expression to **upper case**, using standard ASCII case folding rules.

---

### Examples

```basic
PRINT UPPER$("hello")
```

Produces `"HELLO"`.

```basic
PRINT UPPER$("Retro Rocket 123!")
```

Produces `"RETRO ROCKET 123!"`.

```basic
REM Normalise case for comparison
a$ = "yes"
b$ = "YES"
IF UPPER$(a$) = UPPER$(b$) THEN
    PRINT "They match!"
ENDIF
```

---

### Notes

* Only affects ASCII letters `a–z` (97–122), converting them to `A–Z` (65–90).
* Non-alphabetic characters are unchanged.
* Useful for case-insensitive comparisons, normalisation, and text processing.

---

**See also:**
\ref LOWER "LOWER$" · \ref LEN "LEN" · \ref MID "MID$"
