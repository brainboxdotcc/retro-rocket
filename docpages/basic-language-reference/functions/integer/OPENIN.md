\page OPENIN OPENIN Function

```basic
OPENIN(string-expression)
```

Opens a file for **input** (reading only).
Returns an **integer file handle** that must be used with \ref READ "READ$" and \ref CLOSE "CLOSE".

---

### Examples

```basic
REM Open a file for reading
fh = OPENIN("/data/example.txt")
IF fh < 0 THEN
    PRINT "Failed to open file"
    END
ENDIF

REPEAT
    line$ = READ$(fh)
    IF EOF(fh) = 0 THEN PRINT line$
UNTIL EOF(fh) = 1

CLOSE fh
```

```basic
REM Handle missing file gracefully
fh = OPENIN("/notfound.txt")
IF fh < 0 THEN
    PRINT "File not found"
ELSE
    PRINT "File opened successfully"
    CLOSE fh
ENDIF
```

---

### Notes

* Returns a **non-negative handle** on success, or a **negative value** on failure.
* File handle must be closed with \ref CLOSE "CLOSE" when no longer needed.
* Works with \ref EOF "EOF" to detect end-of-file during reads.
* Use \ref OPENOUT "OPENOUT" to create or overwrite files, and \ref OPENUP "OPENUP" to append to existing files.

---

**See also:**
\ref READ "READ$" · \ref CLOSE "CLOSE" · \ref OPENOUT "OPENOUT" · \ref OPENUP "OPENUP" · \ref EOF "EOF"
