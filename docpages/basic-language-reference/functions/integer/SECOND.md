\page SECOND SECOND Function

```basic
numeric-expression = SECOND(boolean-expression)
```

Returns the current second (0–59).  
Pass `TRUE` for local time or `FALSE` for UTC.

Example:

```basic
PRINT SECOND(TRUE)   ' Local second
PRINT SECOND(FALSE)  ' UTC second
```
