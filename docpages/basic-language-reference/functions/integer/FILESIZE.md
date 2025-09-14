\page FILESIZE FILESIZE Function

```basic
FILESIZE(string-expression)
```

Returns the **size of a file** in **bytes**.
The parameter is a path to a file (absolute or relative to \ref CSD "CSD\$").

---

### Examples

```basic
PRINT FILESIZE("/system/webserver/index.html")
```

```basic
REM Guard against missing files
path$ = "data.bin"
IF FILETYPE$(path$) = "file" THEN
    PRINT "Size = "; FILESIZE(path$); " bytes"
ELSE
    PRINT "Not a regular file"
ENDIF
```

---

### Notes

* Raises an error if the path does not exist or refers to a directory.
* Result is a 64-bit integer (bytes).

---

**See also:**
\ref FILETYPE "FILETYPE$" · \ref OPENIN "OPENIN" · \ref READ "READ$" · \ref BINREAD "BINREAD"
