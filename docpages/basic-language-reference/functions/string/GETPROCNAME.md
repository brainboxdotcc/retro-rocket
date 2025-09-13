\page GETPROCNAME GETPROCNAME$ Function

```basic
GETPROCNAME$(integer-expression)
```

Returns the **name of a running process**.
The parameter is an index between `0` and `GETPROCCOUNT - 1`.

---

### Examples

```basic
REM Print all running process names
FOR i = 0 TO GETPROCCOUNT - 1
    PRINT GETPROCNAME$(i)
NEXT
```

```basic
REM Get the first process name
PRINT "Process 0: "; GETPROCNAME$(0)
```

---

### Notes

* The index must be within the valid range.
* Process names are the identifiers assigned when the process was created.
* Combine with other process functions such as \ref GETPROCID "GETPROCID" or \ref GETPROCMEM "GETPROCMEM" for full details.

---

**See also:**
\ref GETPROCCOUNT "GETPROCCOUNT" · \ref GETPROCID "GETPROCID" · \ref GETPROCMEM "GETPROCMEM" · \ref GETPROCPARENT "GETPROCPARENT" · \ref GETPROCCPUID "GETPROCCPUID"
