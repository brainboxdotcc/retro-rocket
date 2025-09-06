\page DAY DAY Function

```basic
numeric-expression = DAY(boolean-expression)
```

Returns the current day of the month (1–31).  
Pass `TRUE` to return local time using the configured time zone (if any), or `FALSE` for UTC.

Example:

```basic
PRINT DAY(TRUE)   ' Local day
PRINT DAY(FALSE)  ' UTC day
```
