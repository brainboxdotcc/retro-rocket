\page UNTIL UNTIL Keyword
```basic
REPEAT
    statements
UNTIL expression
```

Marks the **end** of a `REPEAT` loop and performs the **post-test** that decides whether to loop again.  
The block between `REPEAT` and `UNTIL` always runs **at least once**. After it runs, `expression` is evaluated:

- If `expression` is **true**, the loop **ends** and execution continues after `UNTIL`.
- If `expression` is **false**, control returns to the matching `REPEAT`.


\remark `UNTIL` must match the **nearest preceding** `REPEAT` and should appear on its **own line**.
@note `REPEAT…UNTIL` blocks may be **nested** freely.

---

### How to think about it
- Read it as **“do these statements until the condition becomes true”** (exit-on-true).
- If you want an **entry-test** loop instead, use [`WHILE … ENDWHILE`](https://github.com/brainboxdotcc/retro-rocket/wiki/WHILE).

---

### Examples

**Count until a limit**
```basic
N = 0
REPEAT
    PRINT "N = "; N
    N = N + 1
UNTIL N >= 5
```

**Prompt until a non-empty string**
```basic
NAME$ = ""
REPEAT
    PRINT "Enter your name:";
    INPUT NAME$
UNTIL LEN(NAME$) > 0
PRINT "Hello, "; NAME$
```

**Wait until a key is pressed**
```basic
PRINT "Press any key to stop."
REPEAT
    REM do some periodic work here
UNTIL INKEY$ <> ""
```

**Nested loops**
```basic
Y = 0
REPEAT
    X = 0
    REPEAT
        PRINT "X="; X; ", Y="; Y
        X = X + 1
    UNTIL X = 3
    Y = Y + 1
UNTIL Y = 2
```

---

### Notes
- The condition after `UNTIL` is an ordinary boolean **expression**; use comparisons like `=`, `<>`, `<`, `>`, `<=`, `>=`.
- Because the test is **after** the body, the body runs **once even if the condition is initially true**.
- `REPEAT…UNTIL` works alongside other control structures (`FOR…NEXT`, `IF…ENDIF`, `WHILE…ENDWHILE`) and may be nested within them.

**See also:**  
\ref REPEAT "REPEAT" ·
\ref WHILE "WHILE" ·
\ref LEN "LEN" ·
\ref INKEY "INKEY$"
