\page ENDWHILE ENDWHILE Keyword
```basic
WHILE expression
    ...
ENDWHILE
```
Marks the end of a `WHILE..ENDWHILE` loop. See [WHILE](https://github.com/brainboxdotcc/retro-rocket/wiki/WHILE). The `ENDWHILE` keyword must match a previous `WHILE`. `WHILE..ENDWHILE` loops may be nested and also contain other forms of loops.

#### Example:

```basic
PRINT "START"
N = 0
WHILE N < 10
    PRINT "IN OUTER WHILE, N = "; N
    O = 0
    WHILE O < 2
        PRINT "IN INNER WHILE, O = "; O
        O = O + 1
    ENDWHILE
    N = N + 1
ENDWHILE
PRINT "LOOPS DONE"
```