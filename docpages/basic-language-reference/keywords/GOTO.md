\page GOTO GOTO Keyword
```basic
GOTO numeric-constant
```

Transfers control unconditionally to the line with the given **constant line number** in the current program.


\remark **Deprecated / discouraged.**
\remark Prefer structured flow control and procedures:
\remark \ref IF "IF",
\remark [`WHILE`/`ENDWHILE`](https://github.com/brainboxdotcc/retro-rocket/wiki/WHILE),
\remark [`REPEAT`/`UNTIL`](https://github.com/brainboxdotcc/retro-rocket/wiki/REPEAT),
\remark [`FOR`/`NEXT`](https://github.com/brainboxdotcc/retro-rocket/wiki/FOR),
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

##### Notes
- Jumping to a non-existent line raises a runtime error.
- Heavy use of `GOTO` quickly reduces readability; use it only for compatibility with legacy code.

**See also:**  
\ref GOSUB "GOSUB" ·
\ref RETURN "RETURN" ·
\ref DEF "DEF"
