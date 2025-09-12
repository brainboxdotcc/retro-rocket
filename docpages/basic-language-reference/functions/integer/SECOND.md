\page SECOND SECOND Function

```basic
SECOND(boolean-expression)
```

Returns the current **second** of the current minute, in the range **0–59**.

* Pass `TRUE` → local time (system time zone, if configured).
* Pass `FALSE` → UTC (Coordinated Universal Time).

---

### Examples

```basic
PRINT "Local second = "; SECOND(TRUE)
PRINT "UTC second   = "; SECOND(FALSE)
```

```basic
REM Display full time as HH:MM:SS
PRINT HOUR(TRUE); ":"; MINUTE(TRUE); ":"; SECOND(TRUE)
```

```basic
REM Wait until the top of the minute
REPEAT
UNTIL SECOND(TRUE) = 0
PRINT "Minute just rolled over!"
```

---

### Notes

* Values range from `0` to `59`.
* Local time depends on the configured time zone. If none is set, local and UTC values are the same.
* Useful with \ref HOUR "HOUR" and \ref MINUTE "MINUTE" for complete time display.

---

**See also:**
\ref HOUR "HOUR" · \ref MINUTE "MINUTE" · \ref DAY "DAY" · \ref MONTH "MONTH" · \ref YEAR "YEAR"
