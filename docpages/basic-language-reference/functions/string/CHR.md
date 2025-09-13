\page CHR CHR$ Function

```basic
CHR$(integer-expression)
```

Returns the ASCII character corresponding to the given integer value.

---

**Notes**

* Valid values are in the range `0`–`255`.
* Values outside this range are truncated to the low byte.
* The return value is always a single character string.

---

**Errors**

* None.

---

**Examples**

```basic
PRINT CHR$(65)        : REM "A"
PRINT CHR$(10)        : REM Newline
PRINT "HELLO" + CHR$(33)   : REM "HELLO!"
```

---

**See also**
\ref ASC "ASC"
