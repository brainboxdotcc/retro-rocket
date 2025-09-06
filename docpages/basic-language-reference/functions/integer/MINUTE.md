\page MINUTE MINUTE Function

```basic
numeric-expression = MINUTE(boolean-expression)
```

Returns the current minute (0–59).  
Pass `TRUE` for local time or `FALSE` for UTC.

Example:

```basic
PRINT MINUTE(TRUE)   ' Local minute
PRINT MINUTE(FALSE)  ' UTC minute
```
