\page DATE DATE$ Function

```basic
string-expression = DATE$(boolean-expression)
```

Returns a string representing the current date in ISO 8601 format (`YYYY-MM-DD`).

- If `boolean-expression` is `TRUE`, the date is adjusted for the current time zone set using `SETTIMEZONE`, including any applicable daylight saving time (DST).
- If `boolean-expression` is `FALSE`, or if no time zone has been set or loaded, the date is returned in UTC as provided by the system clock.

Example:

```basic
PRINT DATE$(TRUE)   ' Local date with DST if applicable
PRINT DATE$(FALSE)  ' Raw UTC date
```
