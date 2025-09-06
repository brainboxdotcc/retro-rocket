\page ON-ERROR ON ERROR Keyword
```basic
ON ERROR PROCname
ON ERROR OFF
```

Installs or removes a **runtime error handler**.

- `ON ERROR PROCname` redirects errors to the named procedure.  
  The handler is called **once** for the next error; if you want it to handle further errors you must **reinstall** it inside the handler.
- `ON ERROR OFF` removes any installed handler (no-op if none is installed).

When an error occurs:
- Built-ins are set: **ERR$** to the message, **ERR** to `1`, **ERRLINE** to the line where it occurred.
- Your handler `PROCname` is invoked.  
- After the handler returns, **execution continues** at the statement **after** the one that failed, unless the handler terminates the program (for example with `END`).


> Only the two forms above are supported. There is **no** `ON ERROR GOTO`, `RESUME`, or similar.


> The handler is invoked as a **one-off** to prevent recursion.  
> If you want ongoing handling, call `ON ERROR PROCname` again **inside your handler** before it returns.


> Pressing `CTRL+ESC` generates a break-like error that is delivered to your installed handler.  
> Without a handler, the program terminates.

---

### Example: handle and continue
```basic
ON ERROR PROChandle

PRINT "About to raise an error"
ERROR "example failure"
PRINT "Continuing after the error"

END

DEF PROChandle
    PRINT "Handled error: "; ERR$; " at line "; ERRLINE
    ON ERROR PROChandle
ENDPROC
```

---

### Example: temporarily disable handling
```basic
ON ERROR PROChandle

PRINT "Phase 1 (handled)"
ERROR "first failure"

ON ERROR OFF
PRINT "Phase 2 (unhandled)"
ERROR "this will terminate"
END

DEF PROChandle
    PRINT "Handler saw: "; ERR$
    ON ERROR PROChandle
ENDPROC
```

---

### Behaviour
- If another error occurs **while calling** your handler, the built-ins **ERR$**, **ERR**, and **ERRLINE** are set for that error, and normal error handling rules apply.
- Handlers are ordinary procedures: they can call other `PROC`s or `FN`s, examine variables, and decide whether to `END` or to reinstall and continue.
- If you are using line-numbered code, you may use your usual control flow after the handler returns, but prefer structured constructs in new code.

**See also:**  
[`ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ERROR) ·
[`DEF`](https://github.com/brainboxdotcc/retro-rocket/wiki/DEF) ·
[`PROC`](https://github.com/brainboxdotcc/retro-rocket/wiki/PROC) ·
[Builtin Variables](https://github.com/brainboxdotcc/retro-rocket/wiki/Builtin-Variables)
