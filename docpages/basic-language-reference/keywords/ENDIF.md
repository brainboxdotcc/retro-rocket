\page ENDIF ENDIF Keyword
```basic
IF expression THEN
    statements
ENDIF

IF expression THEN
    statements
ELSE
    statements
ENDIF
```

Marks the **end** of a multi-line `IF` block. `ENDIF` may follow either the `THEN` block (no `ELSE`) or the `ELSE` block.


\remark Only `ENDIF` is accepted. `END IF` (with a space) is **not** recognised.

---

##### Behaviour
- A multi-line `IF` begins with `IF expression THEN` and must be closed by a matching `ENDIF`.
- An optional `ELSE` may appear before the closing `ENDIF`.
- `ENDIF` matches the **nearest unmatched** `IF`.


\remark The **single-line** form
\remark `IF expression THEN statement ELSE statement`
\remark does **not** use `ENDIF`.

---

##### Examples

**IF with no ELSE**
```basic
IF SCORE >= 100 THEN
    PRINT "LEVEL UP!"
ENDIF
```

**IF with ELSE**
```basic
IF READY THEN
    PRINT "GO!"
ELSE
    PRINT "WAIT..."
ENDIF
```

**Nested IF blocks**
```basic
IF A > 0 THEN
    IF B > 0 THEN
        PRINT "A and B are positive"
    ELSE
        PRINT "A is positive, B is not"
    ENDIF
ELSE
    PRINT "A is not positive"
ENDIF
```

**See also:**
\ref IF "IF" · \ref ELSE "ELSE"
