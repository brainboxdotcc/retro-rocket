\page type-string String Variables

String variables are represented by any variable with the suffix `$`, e.g. `C$` or `MYVAR$`.
They are stored as **null-terminated C-style sequences** of **8-bit ASCII characters**.

---

### Examples

```basic
name$ = "Retro Rocket"
PRINT "Hello, "; name$
```

```basic
REM Concatenate strings
first$ = "Hello"
second$ = "World"
PRINT first$ + " " + second$
```

```basic
REM Accessing characters
ch$ = LEFT$(name$, 1)
PRINT "First character = "; ch$
```

---

### Notes

* Strings are mutable and can be reassigned at any time.
* Maximum length depends on available memory.
* Operations such as concatenation (`+`), substring (\ref LEFT "LEFT$" / \ref RIGHT "RIGHT$" / \ref MID "MID$"), and trimming (\ref TRIM "TRIM$") are supported.
* Null termination is internal — it is not visible during normal BASIC operations.

---

**See also:**
\ref type-integer "Integer Variables" · \ref type-real "Real Variables" · \ref type-array "Array Variables"
