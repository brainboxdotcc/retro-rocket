\page YDAY YDAY Function

```basic
YDAY(boolean-expression)
```

Returns the **day of the year** as an integer in the range **1–366**.

* Pass `TRUE` → local time (using configured time zone and DST rules).
* Pass `FALSE` → UTC (Coordinated Universal Time).

---

### Examples

```basic
PRINT "Local day of year = "; YDAY(TRUE)
PRINT "UTC day of year   = "; YDAY(FALSE)
```

```basic
REM Show progress through the year
d = YDAY(TRUE)
PRINT "Day "; d; " of the year"
PRINT "That is "; (d * 100) / 365; "% complete"
```

```basic
REM Check for leap year extra day
IF YDAY(FALSE) = 366 THEN
    PRINT "Leap year!"
ENDIF
```

---

### Notes

* Valid range is **1–365** for common years, **1–366** for leap years.
* Local time depends on system configuration. Without time zone setup, local and UTC values are the same.
* Useful for progress counters, calendars, and date calculations.

---

**See also:**
\ref DAY "DAY" · \ref MONTH "MONTH" · \ref YEAR "YEAR" · \ref WEEKDAY "WEEKDAY"
