\page REPEAT REPEAT Keyword
```basic
REPEAT
    statements
UNTIL expression
```

Starts a **post-test loop**. The block between `REPEAT` and `UNTIL` executes **at least once**.  
After the block runs, `expression` is evaluated; if it is **true**, the loop **ends**; if **false**, execution repeats from `REPEAT`.


> `REPEAT` pairs with the **nearest** following `UNTIL`.  
> `UNTIL` must appear on its **own line**.  
> You may **nest** `REPEAT…UNTIL` loops.

---

### How to read it

- Think of it as **“do these statements **until** the condition becomes true”** (exit-on-true).
- If you prefer an **entry-test** loop (continue-on-true), use [`WHILE … ENDWHILE`](https://github.com/brainboxdotcc/retro-rocket/wiki/WHILE).

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

**Nested REPEAT loops**
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

**Loop until a key is pressed**
```basic
PRINT "Press any key to stop."
REPEAT
    REM do some periodic work here
UNTIL INKEY$ <> ""
```

---

### Notes
- The condition on `UNTIL` is a normal boolean **expression**; use comparisons like `=`, `<>`, `<`, `>`, `<=`, `>=`.
- Because the test occurs **after** the body, the body runs **once even if the condition is initially true**.
- You can mix `REPEAT…UNTIL` with other control structures (e.g. place one inside a `FOR` loop) as needed.

**See also:**  
[`WHILE`](https://github.com/brainboxdotcc/retro-rocket/wiki/WHILE) ·
[`ENDIF`](https://github.com/brainboxdotcc/retro-rocket/wiki/ENDIF) ·
[`INPUT`](https://github.com/brainboxdotcc/retro-rocket/wiki/INPUT) ·
[`LEN`](https://github.com/brainboxdotcc/retro-rocket/wiki/LEN) ·
[`INKEY$`](https://github.com/brainboxdotcc/retro-rocket/wiki/INKEY)
