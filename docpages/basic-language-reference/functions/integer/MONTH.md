\page MONTH MONTH Function

```basic
numeric-expression = MONTH(boolean-expression)
```

Returns the current month of the year (1–12).  
Pass `TRUE` to return local time using the configured time zone (if any), or `FALSE` for UTC.

Example:

```basic
PRINT MONTH(TRUE)   ' Local month
PRINT MONTH(FALSE)  ' UTC month
```
