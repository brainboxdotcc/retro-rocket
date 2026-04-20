\page ISPROGRAM ISPROGRAM Function

```basic
V = ISPROGRAM(file$)
```

Returns `TRUE` if the specified file appears to contain a BASIC program, otherwise returns `FALSE`.

The file is inspected by reading up to the first 2048 bytes and checking for valid BASIC text and the presence of at least one recognised keyword.

### Notes

`ISPROGRAM` is a heuristic check and does not fully parse the file.

Only the first 2048 bytes are examined, so unusually structured programs with no keywords in this region may be reported as not being BASIC.

When a new program is executed, an extended form of this check is performed that checks the entire program text instead of the first 2048 bytes.

Files containing non-text or binary data will return `FALSE`.

### Errors

If the file cannot be accessed or read, an error is raised.

### Examples

```basic
IF ISPROGRAM("test") THEN
    PRINT "Looks like a BASIC program"
ELSE
    PRINT "Not a BASIC program"
ENDIF
```

```basic
file$ = "config.txt"
IF NOT ISPROGRAM(file$) THEN
    PRINT file$; " is not executable"
ENDIF
```
