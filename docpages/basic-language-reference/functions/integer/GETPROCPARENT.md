\page GETPROCPARENT GETPROCPARENT Function

```basic
GETPROCPARENT(integer-expression)
```

Returns the **parent process ID (PPID)** of a running process.
The parameter is an **index** in the range `0` to `GETPROCCOUNT - 1`, identifying one of the active processes.

---

### Examples

```basic
REM Print parent PID of every running process
FOR i = 0 TO GETPROCCOUNT - 1
    PRINT "Process "; i; " has parent PID "; GETPROCPARENT(i)
NEXT
```

```basic
REM Get parent of the first process
ppid = GETPROCPARENT(0)
PRINT "Parent of process[0] = "; ppid
```

```basic
REM Find all children of a given PID
target = 100
PRINT "Children of PID "; target; ":"
FOR i = 0 TO GETPROCCOUNT - 1
    IF GETPROCPARENT(i) = target THEN
        PRINT "  PID "; GETPROCID(i); " (index "; i; ")"
    ENDIF
NEXT
```

---

### Notes

* The parent process ID identifies which process created or launched the target process.
* Some processes (such as the system’s initial process) may have no parent; in such cases, the PPID is `0`.
* Passing an index outside `0 … GETPROCCOUNT - 1` raises an **error**.
* Combine with \ref GETPROCID "GETPROCID" and \ref GETPROCNAME "GETPROCNAME\$" to build a process tree.

---

**See also:**
\ref GETPROCCOUNT "GETPROCCOUNT" · \ref GETPROCID "GETPROCID" · \ref GETPROCNAME "GETPROCNAME\$" · \ref GETPROCMEM "GETPROCMEM"
