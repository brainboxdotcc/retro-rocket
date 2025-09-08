\page EVAL EVAL Keyword
```basic
EVAL string-expression
```

Executes a **single complete line of BASIC** supplied in `string-expression` in the **current program's context**.

- The string must form **one valid statement line** (for example `PRINT "Hello"`, `X = 10`, `PROCdemo`).
- On **error**, `ERR$` is set to the error message and `ERR` is set to `1`. Execution then continues at the statement after `EVAL`.
- **Recursive EVAL is not permitted** (code running under `EVAL` may not call `EVAL` again).


\remark If you need a literal double quote inside the evaluated line, insert it using `CHR$(34)`.
\remark For example: `EVAL "PRINT " + CHR$(34) + "Hello" + CHR$(34)`.

---

### Examples

**Print from a generated line**
```basic
EVAL "PRINT " + CHR$(34) + "Hello from EVAL!" + CHR$(34)
```

**Set a variable dynamically**
```basic
NAME$ = "COUNT"
EVAL NAME$ + " = 42"
PRINT COUNT
```

**Call a procedure**
```basic
DEF PROCgreet
    PRINT "Greetings"
ENDPROC

EVAL "PROCgreet"
```

**Error handling (ERR/ERR$ set, program continues)**
```basic
ERR = 0
ERR$ = ""

EVAL "A = 1/0"

IF ERR <> 0 THEN
    PRINT "EVAL failed: "; ERR$
ENDIF
```

---

### Notes
- Provide **exactly one** statement; statement separators are not supported.
- The evaluated line runs with access to the program's current variables, procedures, and functions.
- Use `EVAL` for small, dynamic actions. For larger logic, write normal procedures/functions and call them directly.

**See also:**
\ref DEF "DEF", \ref PROC "PROC", \ref FN "FN", [`ERR`/`ERR$`](https://github.com/brainboxdotcc/retro-rocket/wiki/Builtin-Variables)
