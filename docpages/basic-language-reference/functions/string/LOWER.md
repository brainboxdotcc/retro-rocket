\page LOWER LOWER$ Function

```basic
LOWER$(string-expression)
```

Converts a string expression to **lower case**, using standard ASCII case folding rules.

---

### Examples

```basic
PRINT LOWER$("HELLO")
```

Produces `"hello"`.

```basic
PRINT LOWER$("RetroRocket 123!")
```

Produces `"retrorocket 123!"`.

```basic
REM Normalise case for comparison
a$ = "Yes"
b$ = "YES"
IF LOWER$(a$) = LOWER$(b$) THEN
    PRINT "They match!"
ENDIF
```

---

### Notes

* Only affects ASCII letters `A–Z` (65–90), converting them to `a–z` (97–122).
* Non-alphabetic characters are unchanged.
* Useful for case-insensitive comparisons, normalisation, and text processing.

---

**See also:**
\ref UPPER "UPPER$" · \ref LEN "LEN" · \ref MID "MID$"
