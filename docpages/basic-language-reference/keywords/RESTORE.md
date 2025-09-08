\page RESTORE RESTORE Keyword
```
RESTORE
```

The `RESTORE` statement is **not supported in Retro Rocket BASIC**.

In BBC BASIC IV, `RESTORE` reset the pointer for `READ` so embedded `DATA` could be re-read.  
Retro Rocket BASIC replaces this with **file-based I/O**.


@note Use files to hold your constants and reopen them when you need to read again.
@note `READ$` returns **one character at a time**, so build lines or tokens yourself if required.

---

### Pattern 1: reopen to start reading again

**values.txt**
```
10
20
30
```

**Program**
```basic
DIM A,100
I = 0
LINE$ = ""

REM First pass: read all numbers
FH = OPENIN("values.txt")
WHILE NOT EOF(FH)
    C$ = READ$(FH)
    IF C$ = CHR(10) OR C$ = CHR(13) THEN
        IF LEN(LINE$) > 0 THEN
            I = I + 1
            A(I) = VAL(LINE$)
            LINE$ = ""
        ENDIF
    ELSE
        LINE$ = LINE$ + C$
    ENDIF
ENDWHILE
IF LEN(LINE$) > 0 THEN
    I = I + 1
    A(I) = VAL(LINE$)
ENDIF
CLOSE FH

REM Second pass: simply reopen and read again
FH = OPENIN("values.txt")
REM ... do another read as above ...
CLOSE FH
```

---

### Pattern 2: cache once, reuse many times

Instead of re-reading the file, store values in an array on the first pass and reuse the array.

```basic
DIM A,100
COUNT = 0

REM Load once
FH = OPENIN("values.txt")
LINE$ = ""
WHILE NOT EOF(FH)
    C$ = READ$(FH)
    IF C$ = CHR(10) OR C$ = CHR(13) THEN
        IF LEN(LINE$) > 0 THEN
            COUNT = COUNT + 1
            A(COUNT) = VAL(LINE$)
            LINE$ = ""
        ENDIF
    ELSE
        LINE$ = LINE$ + C$
    ENDIF
ENDWHILE
IF LEN(LINE$) > 0 THEN
    COUNT = COUNT + 1
    A(COUNT) = VAL(LINE$)
ENDIF
CLOSE FH

REM Use many times without touching the file
SUM = 0
I = 1
WHILE I <= COUNT
    SUM = SUM + A(I)
    I = I + 1
ENDWHILE
PRINT "SUM = "; SUM
```

---

### Tips
- Store one value per line for simple parsing, or define your own separators and split by looking for those characters.
- When you only need to go back to the **start** of a file, the simplest approach is **close and reopen** it.

**See also:**  
\ref OPENIN "OPENIN" ·
\ref OPENOUT "OPENOUT" ·
\ref OPENUP "OPENUP" ·
\ref READ "READ$" ·
\ref EOF "EOF" ·
\ref VAL "VAL" ·
\ref LEN "LEN" ·
\ref CHR "CHR"
