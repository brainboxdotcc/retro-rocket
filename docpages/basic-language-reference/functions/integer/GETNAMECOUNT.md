\page GETNAMECOUNT GETNAMECOUNT Function

```basic
GETNAMECOUNT(string-expression)
```

Returns the **number of files** in the directory specified by `string-expression`.

* If the path begins with `/`, it is treated as a **fully qualified path**.
* If it does not, it is treated as **relative to the current working directory**.

---

### Examples

```basic
REM Count files in the current directory
PRINT "Current dir has "; GETNAMECOUNT("") ; " files"
```

```basic
REM Relative path example
PRINT "Logs: "; GETNAMECOUNT("logs")
```

```basic
REM Fully qualified path example
PRINT "Programs: "; GETNAMECOUNT("/programs")
```

```basic
REM Guard against empty directories
IF GETNAMECOUNT("/saves") = 0 THEN
    PRINT "No saved games found"
ENDIF
```

---

### Notes

* Counts only **files**, not subdirectories.
* Returns `0` if the directory exists but has no files.
* Returns `0` if the directory cannot be opened or does not exist.
* The empty string (`""`) refers to the **current directory**.
* Works well with \ref GETNAME\$ "GETNAME\$" to iterate over each filename.

---

**See also:**
\ref GETNAME\$ "GETNAME\$" · \ref OPENIN "OPENIN" · \ref OPENOUT "OPENOUT" · \ref CLOSE "CLOSE"
