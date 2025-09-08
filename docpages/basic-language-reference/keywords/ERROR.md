\page ERROR ERROR Keyword
```basic
ERROR string-expression
```

Raises a **runtime error**.

- Sets the built-in variables **ERR$** (to the message) and **ERR** (to `1`) **when**:
  - the error is raised **inside** an \ref EVAL "EVAL", **or**
  - an error handler has been installed with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR).
- If **no** handler is installed and you are not inside `EVAL`, the program **terminates**.

\remark `ERR` and `ERR$` are described on the
\remark \ref variables page.

---

### Examples

**Uncaught error (program terminates)**

```basic
PRINT "About to fail..."
ERROR "there are owls loose"
PRINT "This line is never reached"
```

**Caught error with `ON ERROR PROC...`**

```basic
ON ERROR PROCerr_handler

PRINT "Doing a risky thing..."
ERROR "there are owls loose"
PRINT "Carrying on after handler"

END

DEF PROCerr_handler
    PRINT "ERROR: "; ERR$
    PRINT "ERR code: "; ERR
ENDPROC
```

In this example, the handler runs, `ERR$` contains the message, and `ERR` is `1`.

---

### Notes
- `ERROR` accepts a **string expression**; use it to describe the failure clearly.
- `ON ERROR` supports only `ON ERROR PROCname` and `ON ERROR OFF`.
- When raised inside `EVAL`, `ERR` and `ERR$` are set; see the `EVAL` page for how control returns to the caller.

**See also:**  
[`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR) ·
\ref EVAL "EVAL" ·
[Builtin Variables](https://github.com/brainboxdotcc/retro-rocket/wiki/Builtin-Variables)
