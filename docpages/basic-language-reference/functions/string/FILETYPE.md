\page FILETYPE FILETYPE$ Function

```basic
FILETYPE$(string-expression)
```

Returns the **type** of the object referred to by the given path.
The result is the string `"file"` if it is a file, or `"directory"` if it is a directory.

---

### Examples

```basic
PRINT FILETYPE$("config.txt")
```

Might produce:

```
file
```

```basic
PRINT FILETYPE$("/home/user")
```

Might produce:

```
directory
```

```basic
REM Check type before processing
path$ = "data"
ft$ = FILETYPE$(path$)
IF ft$ = "file" THEN
    PRINT path$; " is a file"
ENDIF
IF ft$ = "directory" THEN
    PRINT path$; " is a directory"
ENDIF
```

---

### Notes

* The path may be absolute (starting with `/`) or relative to the current working directory (\ref CSD "CSD\$").
* If the object does not exist, an error is raised.
* Only two return values are defined: `"file"` and `"directory"`. Other object types are not distinguished.

---

**See also:**
\ref CSD "CSD$" · \ref GETNAMECOUNT "GETNAMECOUNT" · \ref GETSIZE "GETSIZE"
