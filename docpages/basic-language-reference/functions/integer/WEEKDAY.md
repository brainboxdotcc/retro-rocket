\page WEEKDAY WEEKDAY Function

```basic
numeric-expression = WEEKDAY(boolean-expression)
```

Returns the current day of the week as a number (`0 = Sunday`, `1 = Monday`, ..., `6 = Saturday`).  
Pass `TRUE` to adjust using the current time zone, or `FALSE` for UTC.

Example:

```basic
PRINT WEEKDAY(TRUE)   ' Local weekday
PRINT WEEKDAY(FALSE)  ' UTC weekday
```
