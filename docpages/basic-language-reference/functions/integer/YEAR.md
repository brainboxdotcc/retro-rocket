\page YEAR YEAR Function

```basic
numeric-expression = YEAR(boolean-expression)
```

Returns the current full year (e.g. `2025`).  
Pass `TRUE` to return local time using the configured time zone (if any), or `FALSE` for UTC.

Example:

```basic
PRINT YEAR(TRUE)   ' Local year
PRINT YEAR(FALSE)  ' UTC year
```
