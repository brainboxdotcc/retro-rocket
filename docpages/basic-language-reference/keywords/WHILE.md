\page WHILE WHILE Keyword
```basic
WHILE expression
    statements
ENDWHILE
```

Executes the block while `expression` evaluates to **true**.  
The test happens **before** each iteration, so the body may execute **zero times**.


\remark `WHILE` must be closed with `ENDWHILE`.
> The spaced form `END WHILE` is **not** recognised.

---

### Behaviour

- `ENDWHILE` matches the **nearest unmatched** `WHILE`.
- Loops may be **nested** freely.
- The condition `expression` is **re-evaluated every iteration**.
- If the condition never becomes **false**, the loop will run **forever** (until an error or external break).


> Make sure something inside the loop **changes** the values used by `expression`, or the loop will never terminate.

---

### Examples

**Simple count**
```basic
N = 0
WHILE N < 3
    PRINT "N = "; N
    N = N + 1
ENDWHILE
PRINT "Done"
```

**Zero-iteration case (condition false at start)**
```basic
X = 10
WHILE X < 0
    PRINT "This never prints"
ENDWHILE
PRINT "X was not less than 0"
```

**Nested loops**
```basic
PRINT "START"
N = 0
WHILE N < 3
    PRINT "OUTER N = "; N
    O = 0
    WHILE O < 2
        PRINT "INNER O = "; O
        O = O + 1
    ENDWHILE
    N = N + 1
ENDWHILE
PRINT "LOOPS DONE"
```

---

### Notes
- Use `REPEAT ... UNTIL` if you need a **post-test** loop that runs the body at least once.
- The condition is a normal boolean **expression**; use comparisons like `=`, `<>`, `<`, `>`, `<=`, `>=` and combine them with your available operators.

**See also:**  
[`REPEAT`](https://github.com/brainboxdotcc/retro-rocket/wiki/REPEAT) ·
[`ENDIF`](https://github.com/brainboxdotcc/retro-rocket/wiki/ENDIF)
