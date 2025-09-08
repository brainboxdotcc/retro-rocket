\page PROC PROC Keyword
```basic
PROCname(expression, ...)
```

Calls a **procedure** defined with `DEF PROCname(...) ... ENDPROC`.  
A procedure performs actions and then returns to the statement **after** the call. It does **not** return a value.


\remark There is **no space** between `PROC` and the name.
\remark Example: `PROCdrawBox(10, 5)` not `PROC drawBox(10, 5)`.

- The **number** and **types** of arguments must match the `DEF PROC...` definition.
- Procedures cannot be used inside expressions. For a value, define and call an `FN` function.

---

### Examples

**Define and call a procedure**
```basic
DEF PROChello(NAME$)
    PRINT "Hello, "; NAME$
ENDPROC

PROChello("WORLD")
```

**Procedure that draws using parameters**
```basic
DEF PROCfilledCircle(X, Y, R, COL)
    GCOL COL
    CIRCLE X, Y, R, TRUE
ENDPROC

PROCfilledCircle(160, 120, 40, RGB(0,255,0))
```

**Locals inside a procedure**
```basic
DEF PROCsumDemo(N)
    LOCAL TOTAL = 0
    FOR I = 1 TO N
        LOCAL TOTAL = TOTAL + I
    NEXT
    PRINT "Sum to "; N; " = "; TOTAL
ENDPROC

PROCsumDemo(5)
```

---

### Errors and behaviour

- Calling an **undefined** procedure, using the **wrong number** of arguments, or **mismatched types** raises a runtime error (trap with `ON ERROR PROC...` if desired).
- The end of a procedure is marked by **`ENDPROC`**. Using `ENDPROC` outside a procedure is an error.
- Parameters are available by name inside the body. Use `LOCAL name = expr` to create or update locals that shadow any globals of the same name.

---

**See also:**  
\ref DEF "DEF" ·
\ref ENDPROC "ENDPROC" ·
\ref FN "FN" ·
\ref LOCAL "LOCAL"
