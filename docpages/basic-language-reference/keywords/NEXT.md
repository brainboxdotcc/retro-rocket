\page NEXT NEXT Keyword
```basic
FOR numeric-variable = numeric-expression TO numeric-expression
NEXT

FOR numeric-variable = numeric-expression TO numeric-expression STEP numeric-expression
NEXT
```

Marks the **end of a FOR loop body**. When execution reaches `NEXT`, the loop variable is adjusted by the `STEP` amount (or by 1 if no `STEP` was given) and the loop condition is tested. If the end condition has not been met, control continues with the next iteration; otherwise the loop finishes and execution proceeds after `NEXT`.

- `NEXT` takes **no arguments**.
- A **negative** `STEP` value counts **downwards** towards the lower bound.
- Loops may be **nested**; `NEXT` matches the **nearest** unmatched `FOR`.

---

### Examples

Count up by 1
```basic
FOR I = 1 TO 5
    PRINT "I = "; I
NEXT
```

Count down with a negative step
```basic
FOR X = 5 TO 1 STEP -1
    PRINT X
NEXT
```

Custom step
```basic
FOR N = 0 TO 10 STEP 2
    PRINT N
NEXT
```

Nested loops
```basic
FOR Y = 1 TO 3
    FOR X = 1 TO 2
        PRINT "X="; X; " Y="; Y
    NEXT
NEXT
```

---

**See also:**  
\ref FOR "FOR" ·
\ref CONTINUE "CONTINUE"
