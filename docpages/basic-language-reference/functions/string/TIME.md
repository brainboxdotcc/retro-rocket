\page TIME TIME$ Function

```basic
string-expression = TIME$(boolean-expression)
```

Returns a string in 24-hour format representing the current time.

- If `boolean-expression` is `TRUE`, the time is adjusted for the current time zone set using `SETTIMEZONE`, including any applicable daylight saving time (DST).
- If `boolean-expression` is `FALSE`, or if no time zone has been set or loaded, the time is returned in UTC as provided by the system clock.

Example:

```basic
PRINT TIME$(TRUE)   ' Local time with DST if applicable
PRINT TIME$(FALSE)  ' Raw UTC time
```
