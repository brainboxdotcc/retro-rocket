\page MEMFIND MEMFIND Function

```basic
MEMFIND(start, length, value)
```

Searches a memory region for the first occurrence of a byte value.

Returns the address of the matching byte, or zero if the value is not found.

---

### Examples

```basic
REM Find the first NUL byte in a buffer

P = MEMFIND(BUFFER, 100, 0)

IF P THEN
    PRINT "Found at "; P
ENDIF
```

```basic
REM Extract a NUL-terminated tar filename

ENDPOS = MEMFIND(HEADER, 100, 0)

IF ENDPOS = 0 THEN
    ENDPOS = HEADER + 100
ENDIF

NAME$ = BUFFERTOSTRING$(HEADER, ENDPOS - HEADER)

PRINT NAME$
```

```basic
REM Search for a marker byte

POS = MEMFIND(DATA, SIZE, &FF)

IF POS THEN
    PRINT "Escape byte found at "; POS
ENDIF
```

---

### Notes

* `start` is the starting memory address to search.
* `length` is the number of bytes to scan.
* `value` must be between 0 and 255.
* Returns the address of the first matching byte.
* Returns zero if the value is not found.
* The search is performed using a forward linear scan.

---

### Errors

* MEMFIND: Invalid byte value
* Bad Address

---

**See also:**
\ref PEEK "PEEK" · \ref POKE "POKE" · \ref BUFFERTOSTRINGS "BUFFERTOSTRING$" · \ref STRINGTOBUFFER "STRINGTOBUFFER"

