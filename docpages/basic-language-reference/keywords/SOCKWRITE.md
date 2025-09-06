\page SOCKWRITE SOCKWRITE Keyword
```basic
SOCKWRITE integer-variable, printable
```

Writes data to an **open TCP socket**.  
The first parameter must be an **integer variable** holding a valid socket handle (as created earlier with `CONNECT`).  
The second parameter, `printable`, follows the **same formatting rules as `PRINT`**.

---

### Formatting semantics (same as `PRINT`)

- You may pass **numbers** or **strings**; numbers are converted to text.
- Multiple items can be separated by **`;`** or **`,`**:
  - `;` writes the next item **immediately** with **no extra spacing**, and if it is the **last** thing on the line, **suppresses the newline**.
  - `,` advances to the next **print zone** (tab stop) before writing the next item. This inserts **spaces**, not a literal comma.
- With **no trailing `;` or `,`**, a newline is appended automatically (same newline as `PRINT`).
- If you need protocol-specific line endings (for example **CRLF**), append them explicitly with `CHR$(13) + CHR$(10)`.


> A comma in `SOCKWRITE` does **not** send the character `,`.  
> It inserts spaces to the next print zone.  
> If you need a real comma on the wire, include it in the string:
> ```basic
> SOCKWRITE S, "A,B,C"
> ```

---

### Examples

**Build a line without the implicit newline, then add CRLF**
```basic
REM Assume S is a valid socket handle
SOCKWRITE S, "USER ";
SOCKWRITE S, "guest";
SOCKWRITE S, CHR$(13) + CHR$(10)
```

**Send numbers and strings together**
```basic
SOCKWRITE S, "X="; 
SOCKWRITE S, 42; 
SOCKWRITE S, " Y="; 
SOCKWRITE S, 99; 
SOCKWRITE S, CHR$(10)
```

**Avoid print zones; send a literal CSV**
```basic
REM not: SOCKWRITE S, "alpha", "beta", "gamma"
SOCKWRITE S, "alpha,beta,gamma"
```

**Error handling around a write**
```basic
ON ERROR PROCnet_err
SOCKWRITE S, "PING" + CHR$(10)
ON ERROR OFF
END

DEF PROCnet_err
    PRINT "Write failed: "; ERR$
ENDPROC
```

---

### Notes
- Do **not** pass a literal or expression as the handle; use an integer **variable** that contains the handle.
- After you close the socket with `SOCKCLOSE`, the handle is invalid for further `SOCKWRITE` calls.
- `SOCKWRITE` sends exactly what `PRINT` would have produced for the same arguments; use `CHR$` to embed control characters when needed.

**See also:**  
[`PRINT`](https://github.com/brainboxdotcc/retro-rocket/wiki/PRINT) ·
[`SOCKREAD`](https://github.com/brainboxdotcc/retro-rocket/wiki/SOCKREAD) ·
[`SOCKCLOSE`](https://github.com/brainboxdotcc/retro-rocket/wiki/SOCKCLOSE) ·
[`CONNECT`](https://github.com/brainboxdotcc/retro-rocket/wiki/CONNECT)
