\page DATA DATA Keyword
```
DATA value, value, value
```

The `DATA` statement is **not supported** in Retro Rocket BASIC.

In BBC BASIC IV, `DATA` embedded constants directly in the source and they were consumed with `READ` (and rewound with `RESTORE`). Retro Rocket BASIC replaces this pattern with **file-based I/O** and simple initialisers.


\remark `READ$` in Retro Rocket BASIC returns **one character at a time** from an open
\remark file handle. It does **not** read a whole line. If you need to parse lines or
\remark comma-separated values, build them yourself from characters and then convert
\remark with functions like \ref VAL "VAL".

---

##### Example: load newline-separated integers from a file

Suppose `values.txt` contains:
```
10
20
30
```

Programme:

```basic
DIM A, 100
I = 0
LINE$ = ""

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
```

This accumulates characters until a line break (`CHR(10)` or `CHR(13)`), then converts the buffered text into a number with `VAL`.

---

##### Example: simple inline table (no files)

```basic
DIM A, 3
A(1) = 10
A(2) = 20
A(3) = 30
```

---

##### See also
- \ref OPENIN "OPENIN" – open a file for reading
- \ref OPENOUT "OPENOUT" – open a file for writing (truncate)
- \ref OPENUP "OPENUP" – open a file for update (read/write)
- \ref READ "READ$" – read one character from a file handle
- \ref EOF "EOF" – end-of-file check
- \ref VAL "VAL", \ref LEN "LEN", \ref CHR "CHR"
