\page WRITE WRITE Keyword
```basic
WRITE integer-variable, printable
```

Writes text to an **open file**.  
The first parameter must be an **integer variable** containing a **file handle** obtained from [`OPENOUT`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENOUT) or [`OPENUP`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENUP).  
The second parameter, `printable`, follows the **same formatting semantics as `PRINT`**.

---

### Formatting semantics (same as `PRINT`)
- You may pass **strings** and **numeric expressions**; numbers are converted to text.
- Multiple items can be separated by **`;`** or **`,`**:
  - `;` writes the next item **immediately** with **no extra spacing**.
  - `,` advances to the next **print zone** (tab stop) before writing the next item. This inserts **spaces**, not a literal comma.
- If the line **does not** end with `;` or `,`, a **newline** is written automatically.
- To control line endings explicitly (for example CRLF), append them yourself with `CHR$(13) + CHR$(10)`.


> A comma in `WRITE` does **not** emit the character `,`.  
> If you want actual commas (for CSV), include them in the string:
> ```basic
> WRITE FH, "A,B,C"
> ```


> To include a double quote inside a string, insert it with `CHR$(34)`:
> ```basic
> WRITE FH, CHR$(34) + "quoted" + CHR$(34)
> ```

---

### Examples

**Create a file and write two lines**
```basic
FH = OPENOUT("log.txt")
WRITE FH, "Hello"
WRITE FH, "World"
CLOSE FH
```

**Build a line without the implicit newline, then add CRLF**
```basic
FH = OPENOUT("proto.txt")
WRITE FH, "USER ";
WRITE FH, "guest";
WRITE FH, CHR$(13) + CHR$(10)
CLOSE FH
```

**Numbers and strings together**
```basic
FH = OPENOUT("stats.txt")
WRITE FH, "X="; 42; " Y="; 99
CLOSE FH
```

**Avoid print zones when you mean CSV**
```basic
FH = OPENOUT("data.csv")
REM not: WRITE FH, "alpha", "beta", "gamma"
WRITE FH, "alpha,beta,gamma"
CLOSE FH
```

**Error handling around a write**
```basic
ON ERROR PROCfile_err
FH = OPENOUT("out.txt")
WRITE FH, "line"
CLOSE FH
END

DEF PROCfile_err
    PRINT "Write failed: "; ERR$
ENDPROC
```

---

### Notes
- `integer-variable` must be a **variable** holding a valid handle; do not pass a literal or expression.
- After `CLOSE`, the handle is invalid for further `WRITE` calls.
- `WRITE` does not flush independently; closing the handle ensures data is written.

**See also:**  
[`PRINT`](https://github.com/brainboxdotcc/retro-rocket/wiki/PRINT) ·
[`OPENOUT`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENOUT) ·
[`OPENUP`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENUP) ·
[`CLOSE`](https://github.com/brainboxdotcc/retro-rocket/wiki/CLOSE)
