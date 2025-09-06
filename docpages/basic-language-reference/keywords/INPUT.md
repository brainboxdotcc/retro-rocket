\page INPUT INPUT Keyword
```basic
INPUT variable
```

Pauses the program and waits for **keyboard input**. The entered text is echoed to the terminal and then stored in `variable`.

- `variable` may be **string**, **integer**, or **real**.
- If the input is **not valid** for the requested type (for example, letters when an integer is expected), the variable is set to **0**.
- `INPUT` is **blocking**: execution does not continue until the user presses Enter.

The `INPUT` statement is intentionally simple. For richer line-editing (cursor keys, history, etc.) see the
[ansi](https://github.com/brainboxdotcc/retro-rocket/wiki/ansi) library, which also lets you manage your own I/O loop.


\remark Press `CTRL+ESC` at any time to cancel waiting for input.
\remark Without an error handler, the program terminates.
\remark With an `ON ERROR` handler, control passes there instead.

---

### Examples

**Read a number**
```basic
PRINT "Enter a number:"
INPUT N
PRINT "You typed: "; N
```

**Handle invalid numeric input**
```basic
PRINT "Enter an integer:"
INPUT I
IF I = 0 THEN
    PRINT "That was not a valid integer (or it was zero)."
ELSE
    PRINT "OK: "; I
ENDIF
```

**Read a string**
```basic
PRINT "Enter your name:"
INPUT NAME$
PRINT "Hello, "; NAME$
```

**Prompt until non-zero**
```basic
REPEAT
    PRINT "Enter a non-zero value:"
    INPUT X
UNTIL X <> 0
PRINT "Thanks: "; X
```

---

### Notes
- The entered line is stored **as-is** for string variables (without the trailing newline).
- For numeric variables, parsing is strict; invalid input yields **0**.
- `INPUT` echoes characters to the terminal. For full-screen UI or advanced editing, prefer the
  [ansi](https://github.com/brainboxdotcc/retro-rocket/wiki/ansi) library.
