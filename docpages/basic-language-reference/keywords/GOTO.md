\page GOTO GOTO Keyword
```basic
GOTO numeric-constant
```

Transfers control unconditionally to the line with the given **constant line number** in the current program.


\remark **Deprecated / discouraged.**
\remark Prefer structured flow control and procedures:
\remark \ref IF "IF",
\remark \ref WHILE "WHILE" / \ref ENDWHILE "ENDWHILE",
\remark \ref REPEAT "REPEAT" / \ref UNTIL "UNTIL",
\remark \ref FOR "FOR" / \ref NEXT "NEXT",
\remark and `DEF PROC...`/`ENDPROC` with `PROC` calls.

- The target must be a **numeric constant**; variables or expressions are not allowed.
- Jumps may be forward or backward within the **same program**.

---

##### Examples

**Forward jump**
```basic
10 PRINT "Start"
20 GOTO 100
30 PRINT "This line is skipped"
100 PRINT "Landed at 100"
```

**Simple loop with GOTO**
```basic
10 I = 1
20 PRINT I
30 I = I + 1
40 IF I <= 5 THEN GOTO 20
50 PRINT "Done"
```

---

##### Control-flow behaviour

`GOTO` performs a direct jump and does **not** unwind or restore any control structures.

This means you may jump:

* out of a loop (`FOR`, `WHILE`, `REPEAT`)
* into the body of a loop
* out of a `GOSUB` or `PROC`

Retro Rocket BASIC does not crash in these cases, but the program may later raise a runtime error if
control structures no longer match the current execution state (for example, `NEXT without FOR` or `RETURN without GOSUB`).

This behaviour is defined.

---

##### Notes
- Jumping to a non-existent line raises a runtime error.
- Heavy use of `GOTO` quickly reduces readability; use it only for compatibility with legacy code.

**See also:**  
\ref GOSUB "GOSUB" ·
\ref RETURN "RETURN" ·
\ref DEF "DEF"
