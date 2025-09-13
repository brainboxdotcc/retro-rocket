\page READ READ$ Function

```basic
READ$(integer-expression)
```

Reads a **full line** of text from a file, using the file handle specified by the integer expression.
Reading continues until a **carriage return (newline)** or the **end of file** is reached.

---

### Examples

```basic
REM Open a file and print each line
fh = OPENIN("example.txt")
REPEAT
    line$ = READ$(fh)
    PRINT line$
UNTIL EOF(fh)
CLOSE fh
```

```basic
REM Load first line only
fh = OPENIN("config.ini")
first$ = READ$(fh)
PRINT "First line: "; first$
CLOSE fh
```

---

### Notes

* The handle must be valid and obtained from \ref OPENIN "OPENIN" or \ref OPENUP "OPENUP".
* Lines are terminated by carriage return (newline) markers in the file.
* If the end of file is reached, the string may be empty; use \ref EOF "EOF" to detect this condition.
* Binary files may return unexpected results since `READ$` is line-oriented.

---

**See also:**
\ref OPENIN "OPENIN" · \ref OPENUP "OPENUP" · \ref EOF "EOF" · \ref READ "READ" · \ref WRITE "WRITE"
