\page UPTIMES UPTIME$ Function

```basic
UPTIME$
```

Returns the system **uptime as a human-readable string**.

The value represents how long the system has been running since it was started, formatted into days, hours, minutes, and seconds.

---

### Examples

```basic
REM Print formatted uptime
PRINT "Uptime: "; UPTIME$
```

```basic
REM Include uptime in a status message
PRINT "System has been running for "; UPTIME$
```

```basic
REM Simple log output
PRINT "["; UPTIME$; "] System ready"
```

---

### Notes

* Returns a string such as **"0 days, 6 hours, 5 mins 23 secs"**.
* Intended for display purposes rather than timing.
* For numeric timing or comparisons, use \ref UPSECS "UPSECS" or \ref TICKS "TICKS".

---

**See also:**
\ref UPSECS "UPSECS" · \ref TICKS "TICKS"
