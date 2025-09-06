\page YDAY YDAY Function

```basic
numeric-expression = YDAY(boolean-expression)
```

Returns the day of the year (1–366).  
Pass `TRUE` to use the configured time zone and DST rules, or `FALSE` for UTC.

Example:

```basic
PRINT YDAY(TRUE)   ' Local day of year
PRINT YDAY(FALSE)  ' UTC day of year
```
