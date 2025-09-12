\page MONTH MONTH Function

```basic
MONTH(boolean-expression)
```

Returns the current **month of the year** in the range **1–12**.

* Pass `TRUE` → local time (system time zone, if configured).
* Pass `FALSE` → UTC (Coordinated Universal Time).

---

### Examples

```basic
PRINT "Local month = "; MONTH(TRUE)
PRINT "UTC month   = "; MONTH(FALSE)
```

```basic
REM Print full date as DD/MM/YYYY
PRINT DAY(TRUE); "/"; MONTH(TRUE); "/"; YEAR(TRUE)
```

```basic
REM Check if it’s December
IF MONTH(TRUE) = 12 THEN
    PRINT "Happy Holidays!"
ENDIF
```

---

### Notes

* Returns values in the range **1 = January** to **12 = December**.
* Local time depends on system configuration. If no time zone is set, local and UTC values will be identical.
* Use together with \ref DAY "DAY" and \ref YEAR "YEAR" for complete date information.
* For day of week (Monday–Sunday), use \ref WEEKDAY "WEEKDAY".

---

**See also:**
\ref DAY "DAY" · \ref YEAR "YEAR" · \ref WEEKDAY "WEEKDAY" · \ref HOUR "HOUR"
