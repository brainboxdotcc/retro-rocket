\page TELL TELLFunction

```basic
TELL(file-handle)
```

Returns the current position within an open file, in bytes. If the handle given is invalid, an error will be thrown.

---

### Examples

```basic
F = OPENIN("/system/test.txt")
PRINT TELL(F) ' Prints 0
N = READ(F)
PRINT TELL(F) ' Prints 1
CLOSE F
```

**See also:**
\ref OPENIN "OPENIN" · \ref OPENOUT "OPENOUT" · \ref CLOSE "CLOSE" · \ref SEEK "SEEK"
