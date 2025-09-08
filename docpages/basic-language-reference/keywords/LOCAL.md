\page LOCAL LOCAL Keyword
```basic
LOCAL variable-name = expression
```

Declares or modifies a **local variable** whose value is accessible **only within** the current `PROC`, `FN` or `GOSUB` call.  
When that call returns, variables marked `LOCAL` are discarded.


\remark `LOCAL` must appear **with an assignment**.
\remark `LOCAL` without `= expression` is not valid.


\remark **Parameters** to a `PROC` or `FN` are already **local**.
\remark You assign to parameters **normally** (no `LOCAL` needed or wanted).

---

### Behaviour

- A `LOCAL` assignment creates/uses a **local** variable for **this call only**.  
- If a global of the same name exists, `LOCAL` **shadows** it for this call.  
- Inside the body, an assignment **without** `LOCAL` refers to the **global** variable of that name (if any), not the local created with `LOCAL`.  
  Therefore, if you intend to keep updating the local value, prefix **each** assignment with `LOCAL`.

---

### Examples

Create and update a local in a procedure
```basic
DEF PROCsum_demo
    LOCAL TOTAL = 0
    FOR I = 1 TO 3
        LOCAL TOTAL = TOTAL + I
    NEXT
    PRINT "TOTAL = "; TOTAL
ENDPROC

PROCsum_demo
```

Modify a parameter (no LOCAL needed)
```basic
DEF PROCincrement(N)
    N = N + 1
    PRINT "Inside PROC, N = "; N
ENDPROC

X = 10
PROCincrement(X)
PRINT "After call, X = "; X
```

Shadowing a global, and what happens without LOCAL
```basic
X = 5

DEF PROCwork
    LOCAL X = 99
    PRINT "Local X = "; X
    X = 100          REM this updates the GLOBAL X, not the local one
    PRINT "Local X still = "; X
ENDPROC

PROCwork
PRINT "Global X now = "; X
```

Use inside a GOSUB subroutine (line numbers permitted)
```basic
10 GOSUB 1000
20 END

1000 LOCAL T = 1
1010 LOCAL T = T + 1
1020 PRINT "T = "; T
1030 RETURN
```

---

**See also:**  
\ref DEF "DEF" ·
\ref PROC "PROC" ·
\ref ENDPROC "ENDPROC" ·
\ref FN "FN" ·
\ref GOSUB "GOSUB"
