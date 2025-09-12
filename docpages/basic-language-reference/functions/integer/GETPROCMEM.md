\page GETPROCMEM GETPROCMEM Function

```basic
GETPROCMEM(integer-expression)
```

Returns the **memory usage in bytes** of a running process.
The parameter is an **index** in the range `0` to `GETPROCCOUNT - 1`, identifying one of the active processes.

---

### Examples

```basic
REM Print memory usage of all running processes
FOR i = 0 TO GETPROCCOUNT - 1
    PRINT "Process "; i; " uses "; GETPROCMEM(i); " bytes"
NEXT
```

```basic
REM Check if a process exceeds a memory limit
idx = 2
mem = GETPROCMEM(idx)
IF mem > 1048576 THEN
    PRINT "Process "; idx; " exceeds 1 MB"
ENDIF
```

```basic
REM Sum total memory of all processes
total = 0
FOR i = 0 TO GETPROCCOUNT - 1
    total = total + GETPROCMEM(i)
NEXT
PRINT "Total memory used by all processes = "; total; " bytes"
```

---

### Notes

* Returned value is in **bytes**, as a Retro Rocket BASIC integer.
* Memory usage reflects the process’s working set at the time of the call; it may change dynamically as the process allocates or frees memory.
* Passing an index outside `0 … GETPROCCOUNT - 1` raises an **error**.
* Use alongside \ref GETPROCID "GETPROCID" or \ref GETPROCNAME "GETPROCNAME\$" to relate memory use to specific processes.

---

**See also:**
\ref GETPROCCOUNT "GETPROCCOUNT" · \ref GETPROCID "GETPROCID" · \ref GETPROCNAME "GETPROCNAME\$" · \ref GETPROCCPUID "GETPROCCPUID"
