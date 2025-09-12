\page GETPROCID GETPROCID Function

```basic
GETPROCID(integer-expression)
```

Returns the **process ID (PID)** of a running process.
The parameter is an **index** in the range `0` to `GETPROCCOUNT - 1`, identifying one of the active processes.

---

### Examples

```basic
REM Print PID of every running process
FOR i = 0 TO GETPROCCOUNT - 1
    PRINT "Process "; i; " has PID "; GETPROCID(i)
NEXT
```

```basic
REM Get PID of the first process
pid = GETPROCID(0)
PRINT "PID[0] = "; pid
```

```basic
REM Find a process by PID
target = 123
found = FALSE
FOR i = 0 TO GETPROCCOUNT - 1
    IF GETPROCID(i) = target THEN
        PRINT "Process found at index "; i
        found = TRUE
    ENDIF
NEXT
IF found = FALSE THEN PRINT "Process not running"
```

---

### Notes

* Process IDs are **unique identifiers** assigned by the OS.
* A PID remains valid until its process exits.
* Passing an index outside `0 … GETPROCCOUNT - 1` raises an **error**.
* Use together with \ref GETPROCNAME "GETPROCNAME$" or \ref GETPROCCPUID "GETPROCCPUID" for further process details.

---

**See also:**
\ref GETPROCCOUNT "GETPROCCOUNT" · \ref GETPROCNAME "GETPROCNAME$" · \ref GETPROCCPUID "GETPROCCPUID"
