\page ELSE ELSE Keyword

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

`ELSE` provides the **false branch** of an `IF` decision.  
When the `expression` evaluates to **true**, the `THEN` part runs; otherwise the `ELSE` part runs.


\remark `ELSE` is used **with** `IF` and, in multi-line forms, must be paired with a matching [`ENDIF`](https://github.com/brainboxdotcc/retro-rocket/wiki/ENDIF).
\remark `ELSE` on its own is not a valid statement.

---

### Forms

- **Single-line**  
  `IF expression THEN statement ELSE statement`  
  Both the `THEN` and `ELSE` parts must be **single statements** on the same line.

- **Multi-line without ELSE**  
  ```
  IF expression THEN
      statements
  ENDIF
  ```
  The `statements` run only if `expression` is true.

- **Multi-line with ELSE**  
  ```
  IF expression THEN
      statements-when-true
  ELSE
      statements-when-false
  ENDIF
  ```

---

### Examples

**Single-line selection**
```basic
PRINT IF X > 0 THEN "POSITIVE" ELSE "NON-POSITIVE"
```

**Multi-line with ELSE**
```basic
IF SCORE >= 100 THEN
    PRINT "WELL DONE!"
ELSE
    PRINT "KEEP GOING..."
ENDIF
```

**Nested `IF`/`ELSE`**
```basic
IF A > B THEN
    PRINT "A > B"
ELSE
    IF A = B THEN
        PRINT "A = B"
    ELSE
        PRINT "A < B"
    ENDIF
ENDIF
```

---

### Rules & behaviour

- **Pairing:** In multi-line form, `ELSE` pairs with the **nearest unmatched** `IF`. Always terminate the block with [`ENDIF`](https://github.com/brainboxdotcc/retro-rocket/wiki/ENDIF).
- **Single statement on one line:** The one-line form allows exactly **one** statement after `THEN` and **one** after `ELSE`. To perform multiple actions, use the multi-line form.
- **Nesting:** `IF … THEN … ELSE … ENDIF` blocks may be nested freely.
- **Whitespace:** Keywords may be separated by spaces as shown; keep them on their own lines in multi-line form for clarity.

**See also:** [`IF`](https://github.com/brainboxdotcc/retro-rocket/wiki/IF), [`ENDIF`](https://github.com/brainboxdotcc/retro-rocket/wiki/ENDIF)
