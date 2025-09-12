\page WEEKDAY WEEKDAY Function

```basic
WEEKDAY(boolean-expression)
```

Returns the current **day of the week** as a number:

* `0 = Sunday`
* `1 = Monday`
* `2 = Tuesday`
* `3 = Wednesday`
* `4 = Thursday`
* `5 = Friday`
* `6 = Saturday`

Pass `TRUE` to return the local day (adjusted by system time zone), or `FALSE` for UTC.

---

### Examples

```basic
PRINT "Local weekday = "; WEEKDAY(TRUE)
PRINT "UTC weekday   = "; WEEKDAY(FALSE)
```

```basic
REM Print message if it's the weekend
d = WEEKDAY(TRUE)
IF d = 0 OR d = 6 THEN
    PRINT "It's the weekend!"
ELSE
    PRINT "It's a weekday."
ENDIF
```

```basic
REM Map weekday number to name
DIM dayNames$, 7
dayNames$(0) = "Sunday"
dayNames$(1) = "Monday"
dayNames$(2) = "Tuesday"
dayNames$(3) = "Wednesday"
dayNames$(4) = "Thursday"
dayNames$(5) = "Friday"
dayNames$(6) = "Saturday"

PRINT "Today is "; dayNames$(WEEKDAY(TRUE))
```

---

### Notes

* Returns an integer in the range `0–6`.
* Local time depends on configured time zone. Without configuration, local and UTC values are identical.
* Use together with \ref DAY "DAY", \ref MONTH "MONTH", and \ref YEAR "YEAR" for full date handling.

---

**See also:**
\ref DAY "DAY" · \ref MONTH "MONTH" · \ref YEAR "YEAR" · \ref HOUR "HOUR" · \ref MINUTE "MINUTE" · \ref SECOND "SECOND"
