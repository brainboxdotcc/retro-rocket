\page CONTINUE CONTINUE Keyword

```basic
CONTINUE WHILE
CONTINUE FOR
CONTINUE REPEAT
```

Skips the remainder of the current loop body and continues execution at the start of the next iteration.

* `CONTINUE WHILE` jumps to the corresponding `WHILE` line, re-evaluating the loop condition.
* `CONTINUE FOR` jumps to the corresponding `NEXT` statement, performing the increment and test before continuing.
* `CONTINUE REPEAT` jumps to the corresponding `UNTIL` statement, immediately evaluating the condition.

`CONTINUE` must always be followed by a loop keyword (`WHILE`, `FOR`, or `REPEAT`).
It may only appear inside the corresponding loop type, and loops may be nested.

---

#### Example: CONTINUE WHILE

```basic
N = 0
WHILE N < 5
    N = N + 1
    IF N = 3 THEN CONTINUE WHILE
    PRINT "N = "; N
ENDWHILE
PRINT "DONE"
```

Output:

```
N = 1
N = 2
N = 4
N = 5
DONE
```

---

#### Example: CONTINUE FOR

```basic
FOR I = 1 TO 5
    IF I MOD 2 = 0 THEN CONTINUE FOR
    PRINT "ODD:"; I
NEXT
```

Output:

```
ODD: 1
ODD: 3
ODD: 5
```

---

#### Example: CONTINUE REPEAT

```basic
N = 0
REPEAT
    N = N + 1
    IF N = 2 THEN CONTINUE REPEAT
    PRINT "N = "; N
UNTIL N = 3
```

Output:

```
N = 1
N = 3
```
