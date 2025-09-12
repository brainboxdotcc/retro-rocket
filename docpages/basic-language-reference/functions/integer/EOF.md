\page EOF EOF Function

```basic
EOF(integer-variable)
```

Returns `TRUE` (1) if the **end of file** has been reached on the file handle, otherwise `FALSE` (0).
The parameter must be an **integer variable** containing a valid handle from \ref OPENIN "OPENIN" or \ref OPENOUT "OPENOUT" (for append/verify cases).

---

### Examples

```basic
REM Read a text file line by line
FH = OPENIN("mytext.txt")
IF FH < 0 THEN
    PRINT "Could not open file"
    END
ENDIF

REPEAT
    line$ = READ$(FH)
    IF EOF(FH) = 0 THEN
        PRINT line$
    ENDIF
UNTIL EOF(FH) = 1

CLOSE FH
```

```basic
REM Copy a file (simple text mode)
IN  = OPENIN("input.txt")
OUT = OPENOUT("output.txt")
IF (IN < 0) OR (OUT < 0) THEN
    PRINT "Open failed"
    END
ENDIF

REPEAT
    L$ = READ$(IN)
    IF EOF(IN) = 0 THEN
        WRITE OUT, L$
    ENDIF
UNTIL EOF(IN) = 1

CLOSE IN
CLOSE OUT
```

---

### Notes

* `EOF` reports whether the **next read would be past the end**. After a final `READ$`, it becomes `TRUE`.
* Typical pattern: read with `READ$`, process if `EOF(FH)=0`, loop **until** `EOF(FH)=1`.
* On failure to open, file handles are **negative**; check before use.
* Works with file handles from `OPENIN` (reading). For writing with `OPENOUT`, `EOF` is generally not meaningful.

---

**See also:**
\ref OPENIN "OPENIN" · \ref OPENOUT "OPENOUT" · \ref READ "READ$" · \ref WRITE "WRITE" · \ref CLOSE "CLOSE"
