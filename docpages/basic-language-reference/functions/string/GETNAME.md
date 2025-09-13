\page GETNAME GETNAME$ Function

```basic
GETNAME$(string-expression, integer-expression)
```

Returns the **file name** for an entry in a directory.
The first parameter is a **directory path**, and the second parameter is an **index** between `0` and `GETNAMECOUNT(path$) - 1`.

---

### Examples

```basic
REM List all files in the current directory
dir$ = CSD$
count = GETNAMECOUNT(dir$)
FOR i = 0 TO count - 1
    PRINT GETNAME$(dir$, i)
NEXT
```

```basic
REM Access files in a specific directory
FOR i = 0 TO GETNAMECOUNT("/system") - 1
    PRINT GETNAME$("/system", i)
NEXT
```

---

### Notes

* If the index is outside the valid range, an error is raised.
* The path may be absolute (starting with `/`) or relative to the current working directory (\ref CSD "CSD\$").
* The order of entries is determined by the filesystem and may not be sorted alphabetically.
* Only the name is returned — to distinguish files from directories, use \ref FILETYPE "FILETYPE\$".

---

**See also:**
\ref GETNAMECOUNT "GETNAMECOUNT" · \ref FILETYPE "FILETYPE$" · \ref GETSIZE "GETSIZE" · \ref CSD "CSD$"
