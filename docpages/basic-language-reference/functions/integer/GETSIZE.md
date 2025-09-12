\page GETSIZE GETSIZE Function

```basic
GETSIZE(string-expression, integer-expression)
```

Returns the **size of a file** (in bytes) within a directory.

* `string-expression` → path to the directory (fully qualified or relative to the current working directory).
* `integer-expression` → file index in the range `0 … GETNAMECOUNT(dir$) - 1`, identifying the file in that directory.

---

### Examples

```basic
REM Print size of all files in /programs
dir$ = "/programs"
count = GETNAMECOUNT(dir$)
FOR i = 0 TO count - 1
    PRINT GETNAME$(dir$, i); " = "; GETSIZE(dir$, i); " bytes"
NEXT
```

```basic
REM Get size of the first file in /saves
size = GETSIZE("/saves", 0)
PRINT "First save file size = "; size; " bytes"
```

```basic
REM Calculate total size of a directory
dir$ = "/data"
total = 0
FOR i = 0 TO GETNAMECOUNT(dir$) - 1
    total = total + GETSIZE(dir$, i)
NEXT
PRINT "Total size of /data = "; total; " bytes"
```

---

### Notes

* Size is returned as a Retro Rocket BASIC integer (64-bit signed).
* Only valid for **files**. Subdirectories (if present) report size `0`.
* Index must be in range; otherwise an **error** occurs.
* Use together with \ref GETNAME\$ "GETNAME\$" to list files and obtain their sizes.
* To count files before accessing by index, call \ref GETNAMECOUNT "GETNAMECOUNT".

---

**See also:**
\ref GETNAMECOUNT "GETNAMECOUNT" · \ref GETNAME\$ "GETNAME\$" · \ref OPENIN "OPENIN" · \ref READ\$ "READ\$" · \ref CLOSE "CLOSE"
