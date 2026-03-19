\page ENDPROC ENDPROC Keyword
```basic
ENDPROC
```

Ends execution of the **current procedure** (defined with `DEF PROCname`) and returns to the caller.  
Execution resumes at the statement **after** the `PROCname(...)` call.

---

##### Example: simple procedure

```basic
DEF PROCgreet
    PRINT "Hello from a procedure!"
ENDPROC

PROCgreet
```

---

##### Example: conditional early return

```basic
DEF PROCprint_if_positive(N)
    IF N <= 0 THEN
        ENDPROC
    ENDIF

    PRINT "Value = "; N
ENDPROC

PROCprint_if_positive(5)
PROCprint_if_positive(0)
```

---

##### Rules & behaviour
- `ENDPROC` **must** appear inside a procedure body started with `DEF PROCname`.
- Using `ENDPROC` outside a procedure is an error.
- Functions (`DEF FNname`) do **not** use `ENDPROC`; a function returns with a final line that **begins with `=`**.

##### Loop scope

Loops (`FOR`, `WHILE`, `REPEAT`) are scoped to the current procedure.

Any active loops created inside a `PROC` are discarded when `ENDPROC` is reached unless they have already been exited normally.

If execution continues and a loop-closing statement is encountered with no matching loop, a runtime error is raised (e.g. `NEXT without FOR`).

**See also:**  
\ref DEF "DEF" ·
\ref PROC "PROC" ·
\ref FN "FN"
