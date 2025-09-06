\page GOSUB GOSUB Keyword
```basic
GOSUB numeric-constant
```

Transfers control to the line with the given **constant line number**, saving the return address so that execution can continue at the statement **after** the `GOSUB` when a matching `RETURN` is encountered.


\remark **Deprecated / discouraged.**
\remark Use structured procedures instead:
\remark define with `DEF PROCname(...) ... ENDPROC` and call with `PROCname(...)`.
\remark `GOSUB` is retained for compatibility with legacy, line-numbered code.

- The target must be a **numeric constant** (no expressions or variables).
- The target line must exist **within the current program**.
- Each `GOSUB` must eventually reach a `RETURN`.

---

##### Example: simple subroutine (line numbers permitted)

```basic
10 PRINT "Start"
20 GOSUB 1000
30 PRINT "Back from subroutine"
40 END

1000 PRINT "Inside subroutine"
1010 RETURN
```

---

##### Example: using variables with a subroutine

```basic
10 N = 3
20 GOSUB 2000
30 PRINT "Done"
40 END

2000 PRINT "Counting..."
2010 FOR I = 1 TO N
2020     PRINT I
2030 NEXT
2040 RETURN
```

---

##### Notes
- `GOSUB` does not support arguments. Share data via variables; prefer `PROC` parameters in new code.
- Jumping to a line that does not contain the intended subroutine body will execute whatever is there until a `RETURN` is hit.
- Modern Retro Rocket BASIC code should prefer **`DEF PROC` / `PROC`** for clarity and maintainability.

**See also:**  
[`RETURN`](https://github.com/brainboxdotcc/retro-rocket/wiki/RETURN) ·
[`DEF`](https://github.com/brainboxdotcc/retro-rocket/wiki/DEF) ·
[`PROC`](https://github.com/brainboxdotcc/retro-rocket/wiki/PROC) ·
[`GOTO`](https://github.com/brainboxdotcc/retro-rocket/wiki/GOTO)
