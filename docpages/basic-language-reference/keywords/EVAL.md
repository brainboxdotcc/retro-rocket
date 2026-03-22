\page EVAL EVAL Keyword
```basic
EVAL string-expression
````

Executes BASIC code supplied in `string-expression`.

* If the string contains a **single line**, it is executed in the **current program's context**.
* If the string contains **multiple lines**, it is executed as an **anonymous child program** in a new process context.

On **error**, `ERR$` is set to the error message and `ERR` is set to `1`. Execution then continues at the statement after `EVAL`.

* Recursive **single-line EVAL** is not permitted.

\remark If you need a literal double quote inside the evaluated line, insert it using `CHR$(34)`.
\remark For example: `EVAL "PRINT " + CHR$(34) + "Hello" + CHR$(34)`.

---

### Examples

**Print from a generated line (single-line)**

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

**Multi-line anonymous program**

```basic
EVAL "A=1" + CHR$(10) + "PRINT A"
```

**Using GLOBAL variables in multi-line EVAL**

```basic
GLOBAL A
A = 5

EVAL "PRINT A"
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

* A **single-line** EVAL executes in the current context and has access to all variables, procedures, and functions.
* A **multi-line** EVAL runs as an anonymous child program.
* Anonymous child programs inherit `GLOBAL` variables from the parent.
* Single-line EVAL executes via a temporary injected line; therefore, recursive single-line EVAL is not permitted.
* Provide **exactly one statement** when using single-line EVAL; statement separators are not supported.
* Use EVAL for dynamic or generated code. For larger logic, prefer normal procedures or separate programs.

**See also:**
\ref DEF "DEF", \ref PROC "PROC", \ref FN "FN", `ERR`, `ERR$`
