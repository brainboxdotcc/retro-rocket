\page HOUR HOUR Function

```basic
numeric-expression = HOUR(boolean-expression)
```

Returns the current hour (0–23).  
Pass `TRUE` for local time (respecting time zone and DST), or `FALSE` for UTC.

Example:

```basic
PRINT HOUR(TRUE)   ' Local hour
PRINT HOUR(FALSE)  ' UTC hour
```
