\page FOR FOR Keyword
```basic
FOR numeric-variable = numeric-expression TO numeric-expression
NEXT

FOR numeric-variable = numeric-expression TO numeric-expression STEP numeric-expression
NEXT
```

Creates a counting loop. In the first form, the loop variable increases by 1 each time control reaches `NEXT`.  
In the second form, the loop variable changes by the `STEP` amount each time.

If `STEP` evaluates to a negative value, the variable will decrement towards the lower value.

Loops may be nested. `NEXT` closes the nearest unmatched `FOR`.

---

### Examples

Count up by 1
```basic
FOR I = 1 TO 5
    PRINT "I = "; I
NEXT
```

Count up by a custom step
```basic
FOR N = 0 TO 10 STEP 2
    PRINT N
NEXT
```

Count down with a negative step
```basic
FOR X = 5 TO 1 STEP -1
    PRINT X
NEXT
```

Real step size
```basic
FOR T# = 0 TO 1 STEP 0.25
    PRINT T#
NEXT
```

---

**See also:** [`NEXT`](https://github.com/brainboxdotcc/retro-rocket/wiki/NEXT)
