\page RETURN RETURN Keyword
```basic
RETURN
```

Returns from a **subroutine** that was entered with \ref GOSUB "GOSUB".
Execution resumes at the **statement after** the `GOSUB` call.


@note `RETURN` is only valid while servicing a `GOSUB`.
@note Using `RETURN` **outside** a `GOSUB` context is an error.


@note `GOSUB`/`RETURN` are kept for compatibility and are **deprecated**.
@note Prefer structured procedures: `DEF PROCname(...) ... ENDPROC`, called with `PROCname(...)`.

---

### Behaviour

- `RETURN` takes **no arguments**.
- Nested subroutines are supported: each `GOSUB` must eventually reach a `RETURN`, which returns to its **own** caller.
- `RETURN` is **not** used to exit `PROC` or `FN`:
  - Procedures end with `ENDPROC`.
  - Functions return a value on a line that begins with `=`.

---

### Examples (line numbers permitted)

**Simple subroutine**
```basic
10 PRINT "Start"
20 GOSUB 1000
30 PRINT "Back in main code"
40 END

1000 PRINT "Inside subroutine"
1010 RETURN
```

**Nested GOSUBs**
```basic
10 PRINT "Main"
20 GOSUB 1000
30 PRINT "Done"
40 END

1000 PRINT "First level"
1010 GOSUB 2000
1020 PRINT "Back to first level"
1030 RETURN

2000 PRINT "Second level"
2010 RETURN
```

**Error: RETURN without GOSUB (do not do this)**
```basic
10 RETURN
```

---

### See also
- \ref GOSUB "GOSUB" - jump to a line and return
- \ref DEF "DEF", \ref PROC "PROC", \ref ENDPROC "ENDPROC" - structured alternatives
