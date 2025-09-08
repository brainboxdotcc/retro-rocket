\page DEF DEF Keyword
```basic
DEF FNname(param, ...)
DEF PROCname(param, ...)
    ...
ENDPROC
```

Defines a **function** (`FN`) or **procedure** (`PROC`). The parameter list is optional.

- A **function** must end with a line that **begins with `=`**; the expression after `=` is the return value.
- A **procedure** ends with **`ENDPROC`** (it does not return a value).


\remark There is **no space** between `FN`/`PROC` and the identifier - in both **definitions** and **calls**.
\remark Examples: `DEF FNadd(A,B)`, `PRINT FNadd(1,2)`, `DEF PROCbox(M$)`, `PROCbox("Hi")`


\remark `GLOBAL` is **only** used to pass variables into **child programs** spawned with
\remark \ref CHAIN "CHAIN".
\remark It does **not** declare "global variables" for program-wide scope inside a single program.
\remark Use parameters and \ref LOCAL "LOCAL" to manage scope within your own code.

---

### Calling functions and procedures

- Call a **function** within an expression:
  ```basic
  RESULT = FNadd(10, 32)
  ```
- Invoke a **procedure** as a statement:
  ```basic
  PROCbox("HELLO")
  ```

---

### Example: function (returns a value)

```basic
DEF FNadd(A, B)
    SUM = A + B
= SUM

PRINT FNadd(1, 2)    : REM prints 3
```

The final line starts with `=` and supplies the return value of the function.

---

### Example: procedure (no return value)

```basic
DEF PROCbox(MSG$)
    PRINT "**************"
    PRINT "* "; MSG$; " *"
    PRINT "**************"
ENDPROC

PROCbox("HELLO")
```

`ENDPROC` marks the end of the procedure body.

---

### Example: locals within a definition

```basic
DEF FNhypot(X, Y)
    LOCAL XSQ = X * X
    LOCAL YSQ = Y * Y
= SQRT(XSQ + YSQ)
```

`LOCAL` creates temporary variables scoped to the definition.

---

### Rules and behaviour

- **Parameters**  
  The list may be empty. Parameters are available by name in the body.  
  Use `LOCAL` for additional temporaries.

- **Function result**  
  Inside a `DEF FN...` block, the **last line** must start with `=` followed by an expression.  
  Using a leading `=` outside a function is an error.

- **Procedure termination**  
  Procedures must end with `ENDPROC`. Using `ENDPROC` outside a procedure is an error.

- **Errors**  
  Calling an undefined `FN`/`PROC`, or passing the wrong number of arguments, raises a runtime error  
  (catchable with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR)).

**See also:**  
\ref FN "FN" ·
\ref PROC "PROC" ·
\ref ENDPROC "ENDPROC" ·
\ref LOCAL "LOCAL" ·
\ref CHAIN "CHAIN" ·
[`Variable Naming`](https://github.com/brainboxdotcc/retro-rocket/wiki/Variable-Naming) ·
[`Parameter types`](https://github.com/brainboxdotcc/retro-rocket/wiki/Parameter-types)
