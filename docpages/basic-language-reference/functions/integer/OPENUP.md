\page OPENUP OPENUP Function

```basic
OPENUP(string-expression)
```

Opens a file for **input and output** (reading and writing).
Returns an **integer file handle** that must be used with \ref READ\$ "READ\$", \ref WRITE "WRITE", and \ref CLOSE "CLOSE".

---

### Examples

```basic
REM Open an existing file for update
fh = OPENUP("/data/config.txt")
IF fh < 0 THEN
    PRINT "Could not open file"
    END
ENDIF

REM Read the first line
line$ = READ$(fh)
PRINT "First line: "; line$

REM Write new content
WRITE fh, "Updated value"
CLOSE fh
```

```basic
REM Create a file if it doesn't exist
fh = OPENUP("/data/newfile.txt")
IF fh >= 0 THEN
    WRITE fh, "Initial content"
    CLOSE fh
ENDIF
```

---

### Notes

* Provides **both read and write access** to the file.
* Returns a **non-negative handle** on success, or a **negative value** on failure.
* Unlike \ref OPENOUT "OPENOUT", this does not always truncate the file; content may be preserved.
* File handle must be closed with \ref CLOSE "CLOSE".
* Use with \ref EOF "EOF" to detect the end of file during reads.

---

**See also:**
\ref READ "READ\$" · \ref WRITE "WRITE" · \ref CLOSE "CLOSE" · \ref OPENIN "OPENIN" · \ref OPENOUT "OPENOUT"
