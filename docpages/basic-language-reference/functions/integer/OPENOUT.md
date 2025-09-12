\page OPENOUT OPENOUT Function

```basic
OPENOUT(string-expression)
```

Opens a file for **output** (writing).
Returns an **integer file handle** that must be used with \ref WRITE "WRITE" and \ref CLOSE "CLOSE".

---

### Examples

```basic
REM Open a file for writing
fh = OPENOUT("/data/output.txt")
IF fh < 0 THEN
    PRINT "Failed to open file"
    END
ENDIF

WRITE fh, "Hello from Retro Rocket!"
CLOSE fh
```

```basic
REM Overwrite an existing file
fh = OPENOUT("/data/log.txt")
WRITE fh, "New log entry"
CLOSE fh
```

---

### Notes

* If the file already exists, it is **truncated** to zero length.
* If the file does not exist, it is **created**.
* Returns a **non-negative handle** on success, or a **negative value** on failure.
* File handle must be closed with \ref CLOSE "CLOSE".
* For read/write access, see \ref OPENUP "OPENUP".

---

**See also:**
\ref WRITE "WRITE" · \ref CLOSE "CLOSE" · \ref OPENIN "OPENIN" · \ref OPENUP "OPENUP"
