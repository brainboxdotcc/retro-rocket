\page GETPROCCPUID GETPROCCPUID Function

```basic
GETPROCCPUID(integer-expression)
```

Returns the **CPU ID** on which the specified process is currently running.
The parameter is an **index** in the range `0` to `GETPROCCOUNT - 1`, identifying one of the active processes.

---

### Examples

```basic
REM Print CPU ID for every running process
FOR i = 0 TO GETPROCCOUNT - 1
    PRINT "Process "; i; " is on CPU "; GETPROCCPUID(i)
NEXT
```

```basic
REM Monitor a specific process
idx = 2
PRINT "Process "; idx; " CPU ID: "; GETPROCCPUID(idx)
```

```basic
REM Check if any process is bound to CPU 0
found = FALSE
FOR i = 0 TO GETPROCCOUNT - 1
    IF GETPROCCPUID(i) = 0 THEN
        found = TRUE
    ENDIF
NEXT
IF found = TRUE THEN PRINT "At least one process on CPU 0"
```

---

### Notes

* CPU IDs are **zero-based**:

  * `0` → first CPU core
  * `1` → second CPU core, etc.
* The mapping can change as the scheduler moves processes between CPUs.
* Passing an index outside the valid range (`0 … GETPROCCOUNT - 1`) raises an **error**.
* Useful for observing process distribution in multi-core systems.
* To get process names or IDs instead of CPU IDs, use \ref GETPROCNAME\$ "GETPROCNAME\$" or \ref GETPROCID "GETPROCID".

---

**See also:**
\ref GETPROCCOUNT "GETPROCCOUNT" · \ref GETPROCNAME\$ "GETPROCNAME\$" · \ref GETPROCID "GETPROCID"
