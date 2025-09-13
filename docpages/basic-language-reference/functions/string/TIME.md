\page TIME TIME$ Function

```basic
string-expression = TIME$(boolean-expression)
```

Returns the current time as a string in **24-hour format** (`HH:MM:SS`).

* If `boolean-expression` is `TRUE`, the time is adjusted for the current time zone configured with \ref SETTIMEZONE, including daylight saving time (DST) if applicable.
* If `boolean-expression` is `FALSE`, or if no time zone has been set or loaded, the time is returned in **UTC** from the system clock.

---

### Examples

```basic
PRINT TIME$(TRUE)
```

Might produce:

```
14:37:05
```

```basic
PRINT TIME$(FALSE)
```

Might produce:

```
13:37:05
```

---

### Notes

* Returned format is always fixed-width `HH:MM:SS`, with leading zeros where necessary.
* Local time depends on whether a valid time zone has been configured via \ref SETTIMEZONE
* DST adjustments are handled automatically when local time is requested.
* For numeric access to components of the current time, see \ref HOUR "HOUR", \ref MINUTE "MINUTE", \ref SECOND "SECOND".

---

**See also:**
\ref HOUR "HOUR" · \ref MINUTE "MINUTE" · \ref SECOND "SECOND" · \ref DAY "DAY" · \ref YEAR "YEAR"
