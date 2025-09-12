\page DAY DAY Function

```basic
DAY(boolean-expression)
```

Returns the current **day of the month** (1–31).

* Pass `TRUE` → local time (system time zone, if configured).
* Pass `FALSE` → UTC (Coordinated Universal Time).

---

### Examples

```basic
REM Print both local and UTC day
PRINT "Local day: "; DAY(TRUE)
PRINT "UTC day:   "; DAY(FALSE)
```

```basic
REM Simple log timestamp
PRINT "Today is "; DAY(TRUE); "/"; MONTH(TRUE); "/"; YEAR(TRUE)
```

```basic
REM Check for month end
IF DAY(FALSE) = 31 THEN
    PRINT "Last day of a long month (UTC)"
ENDIF
```

---

### Notes

* Returns values in the range **1–31**, depending on the current month.
* Local time behaviour depends on the OS time zone configuration. If no zone is configured, local and UTC values will be identical.
* Works in tandem with \ref MONTH "MONTH" and \ref YEAR "YEAR" for complete date values.
* Unlike some BASIC dialects, `DAY` does **not** return day-of-week; use \ref WEEKDAY "WEEKDAY" instead.

---

**See also:**
\ref MONTH "MONTH" · \ref YEAR "YEAR" · \ref WEEKDAY "WEEKDAY"
