\page HOUR HOUR Function

```basic
HOUR(boolean-expression)
```

Returns the current **hour of the day** in the range **0–23**.

* Pass `TRUE` → local time (using configured time zone and daylight saving rules, if any).
* Pass `FALSE` → UTC (Coordinated Universal Time).

---

### Examples

```basic
PRINT "Local hour = "; HOUR(TRUE)
PRINT "UTC hour   = "; HOUR(FALSE)
```

```basic
REM Simple clock demo
REPEAT
    CLS
    PRINT "The time is "; HOUR(TRUE); ":"; MINUTE(TRUE)
    WAIT 50
UNTIL FALSE
```

```basic
REM Decide greeting based on time
h = HOUR(TRUE)
IF h < 12 THEN
    PRINT "Good morning!"
ELSEIF h < 18 THEN
    PRINT "Good afternoon!"
ELSE
    PRINT "Good evening!"
ENDIF
```

---

### Notes

* The range is **0–23**; there is no AM/PM indicator.
* Local time depends on the system time zone configuration. If none is set, local and UTC values are identical.
* Works with \ref MINUTE "MINUTE" and \ref SECOND "SECOND" for more precise timing.
* For date information, use \ref DAY "DAY", \ref MONTH "MONTH", and \ref YEAR "YEAR".

---

**See also:**
\ref MINUTE "MINUTE" · \ref SECOND "SECOND" · \ref DAY "DAY" · \ref MONTH "MONTH" · \ref YEAR "YEAR"
