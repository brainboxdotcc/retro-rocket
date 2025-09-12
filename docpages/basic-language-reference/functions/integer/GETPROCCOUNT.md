\page GETPROCCOUNT GETPROCCOUNT Function

```basic
GETPROCCOUNT
```

Returns the **number of processes** currently running in the system.
This includes both user programs and background tasks managed by Retro Rocket OS.

---

### Examples

```basic
REM Print number of active processes
PRINT "Running processes: "; GETPROCCOUNT
```

```basic
REM Use process count in a conditional
IF GETPROCCOUNT > 10 THEN
    PRINT "System busy"
ELSE
    PRINT "System idle"
ENDIF
```

```basic
REM Example: wait until background process finishes
start_count = GETPROCCOUNT
REPEAT
    PROCdoSomething
UNTIL GETPROCCOUNT = start_count
PRINT "Background job ended"
```

---

### Notes

* The returned value is an **integer** ≥ 1 (the current program itself counts as a process).
* Values change dynamically as processes are created or end.
* Designed for diagnostic or monitoring use; it does not reveal details of which processes are running — only the **total count**.
* Use together with \ref GETPROCNAME\$ "GETPROCNAME\$" or \ref GETPROCID "GETPROCID" to identify individual processes.

---

**See also:**
\ref GETPROCNAME\$ "GETPROCNAME\$" · \ref GETPROCID "GETPROCID" · \ref KILL "KILL"
