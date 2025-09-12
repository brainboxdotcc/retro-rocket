\page READ READ Function

```basic
READ(integer-variable)
```

Reads a single **byte value** from an **open file**.
The parameter is the **integer file handle** returned by \ref OPENIN "OPENIN", \ref OPENOUT "OPENOUT", or \ref OPENUP "OPENUP".
The function returns the next byte as an integer in the range `0–255`.

---

### Examples

```basic
REM Read a file byte by byte
fh = OPENIN("/data/example.bin")
IF fh < 0 THEN
    PRINT "Could not open file"
    END
ENDIF

REPEAT
    b = READ(fh)
    IF EOF(fh) = 0 THEN PRINT "Byte = "; b
UNTIL EOF(fh) = 1

CLOSE fh
```

```basic
REM Read first three bytes
fh = OPENIN("/data/file.bin")
b1 = READ(fh)
b2 = READ(fh)
b3 = READ(fh)
PRINT b1; ","; b2; ","; b3
CLOSE fh
```

---

### Notes

* Returns an integer `0–255`.
* Use \ref EOF "EOF" to detect end-of-file. If called at EOF, behaviour is undefined without checking.
* For reading full lines as strings, use \ref READ "READ$".
* File handle must be closed with \ref CLOSE "CLOSE".

---

**See also:**
\ref READ "READ\$" · \ref OPENIN "OPENIN" · \ref OPENUP "OPENUP" · \ref CLOSE "CLOSE" · \ref EOF "EOF"
