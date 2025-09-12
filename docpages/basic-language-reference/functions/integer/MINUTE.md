\page MINUTE MINUTE Function

```basic
MINUTE(boolean-expression)
```

Returns the current **minute of the hour** in the range **0–59**.

* Pass `TRUE` → local time (system time zone, if configured).
* Pass `FALSE` → UTC (Coordinated Universal Time).

---

### Examples

```basic
PRINT "Local minute = "; MINUTE(TRUE)
PRINT "UTC minute   = "; MINUTE(FALSE)
```

```basic
REM Show current time in HH:MM
PRINT HOUR(TRUE); ":"; MINUTE(TRUE)
```

```basic
REM Trigger an action exactly on the half-hour
IF MINUTE(TRUE) = 30 THEN
    PRINT "It's half-past the hour!"
ENDIF
```

---

### Notes

* Values range from `0` to `59`.
* Local time depends on the configured time zone. Without configuration, local and UTC values will be identical.
* Works with \ref HOUR "HOUR" and \ref SECOND "SECOND" for finer time resolution.

---

**See also:**
\ref HOUR "HOUR" · \ref SECOND "SECOND" · \ref DAY "DAY" · \ref MONTH "MONTH" · \ref YEAR "YEAR"
