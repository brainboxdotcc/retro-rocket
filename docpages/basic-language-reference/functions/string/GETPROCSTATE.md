\page GETPROCSTATE GETPROCSTATE$ Function

```basic
GETPROCSTATE$(integer-expression)
```

Returns the **state of a running process** as a string.
The parameter is an index between `0` and `GETPROCCOUNT - 1`.

---

### Process states

| State       | Description                                                       |
| ----------- | ----------------------------------------------------------------- |
| `running`   | Process is actively executing.                                    |
| `suspended` | Process is waiting on another process to end before continuing.   |
| `waiting`   | Process is blocked, waiting on I/O (e.g. disk, console, network). |
| `ended`     | Process no longer exists.                                         |
| `unknown`   | Error state - the status of this process cannot be determined.    |

---

### Examples

```basic
REM Print all processes with their state
FOR i = 0 TO GETPROCCOUNT - 1
    PRINT GETPROCNAME$(i); " : "; GETPROCSTATE$(i)
NEXT
```

```basic
REM Check if the first process has ended
IF GETPROCSTATE$(0) = "ended" THEN
    PRINT "Process 0 is finished."
ENDIF
```

---

### Notes

* The index must be within the valid range.
* A process that reports `"ended"` is still counted in `GETPROCCOUNT` until it is fully reaped.
* `"unknown"` is rare and usually indicates an internal error in process tracking.

---

**See also:**
\ref GETPROCCOUNT "GETPROCCOUNT" · \ref GETPROCNAME "GETPROCNAME$" · \ref GETPROCID "GETPROCID" · \ref GETPROCMEM "GETPROCMEM" · \ref GETPROCPARENT "GETPROCPARENT"
