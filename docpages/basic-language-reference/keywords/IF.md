\page IF IF Keyword
```basic
IF expression THEN statement ELSE statement

IF expression THEN
    statements
ENDIF

IF expression THEN
    statements
ELSE
    statements
ENDIF
```

Executes statements if an expression evaluates to **true**. You can write `IF` as a **single-line** statement (with one statement after `THEN` and, optionally, one after `ELSE`), or as a **multi-line** block closed by [`ENDIF`](https://github.com/brainboxdotcc/retro-rocket/wiki/ENDIF).


\remark Multi-line blocks must be closed with `ENDIF`.
\remark `END IF` (with a space) is **not** recognised.

---

### Comparison operators

| Operator | Meaning                    |
|:-------:|-----------------------------|
| `>`     | Greater than                |
| `<`     | Less than                   |
| `>=`    | Greater than or equal to    |
| `<=`    | Less than or equal to       |
| `=`     | Equal to                    |
| `<>`    | Not equal to                |

These operators produce a boolean result suitable for `IF`.

---

### Forms and examples

**Single-line (with ELSE)**
```basic
IF X > 0 THEN PRINT "POSITIVE" ELSE PRINT "NON-POSITIVE"
```

**Single-line (no ELSE)**
```basic
IF READY THEN PRINT "GO"
```

**Multi-line (no ELSE)**
```basic
IF SCORE >= 100 THEN
    PRINT "LEVEL UP"
ENDIF
```

**Multi-line (with ELSE)**
```basic
IF NAME$ = "ADMIN" THEN
    PRINT "WELCOME"
ELSE
    PRINT "ACCESS DENIED"
ENDIF
```

**Nested IF**
```basic
IF A > 0 THEN
    IF B > 0 THEN
        PRINT "BOTH POSITIVE"
    ELSE
        PRINT "A POSITIVE ONLY"
    ENDIF
ELSE
    PRINT "A NOT POSITIVE"
ENDIF
```

---

### Notes
- In the **single-line** form, the `THEN` and optional `ELSE` parts each allow **one statement**. Use the multi-line form for multiple statements.
- `IF` blocks may be **nested** freely; `ENDIF` matches the nearest unmatched `IF`.
- Use parentheses to make complex conditions explicit: `IF (A > 0) AND (B < 10) THEN ...` (where supported by your operators).

**See also:** [`ELSE`](https://github.com/brainboxdotcc/retro-rocket/wiki/ELSE) · [`ENDIF`](https://github.com/brainboxdotcc/retro-rocket/wiki/ENDIF)
