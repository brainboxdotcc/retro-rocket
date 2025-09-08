\page FN FN Keyword
```basic
FNname(expression, ...)
```

Calls a function defined with `DEF FNname(...)`. The call appears inside an expression (for example on the right-hand side of `=`, or as part of `PRINT`).

- There is no space between `FN` and the function name (e.g. `FNadd(1,2)`, not `FN add(1,2)`).
- The number of arguments and their types must match the definition.
- The return type is determined by the function name suffix:
  - no suffix  - integer
  - name ending with `#` - real
  - name ending with `$` - string
- A function defined with `DEF FN...` returns its value on a line that begins with `=` in the definition.

---

### Examples

Integer result
```basic
DEF FNadd(A, B)
= A + B

X = FNadd(2, 3)
PRINT X
```

Real result
```basic
DEF FNmean#(A#, B#)
= (A# + B#) / 2#

M# = FNmean#(1.5#, 2.5#)
PRINT M#
```

String result
```basic
DEF FNgreet$(NAME$)
= "HELLO, " + NAME$

PRINT FNgreet$("WORLD")
```

---

### Errors

- Calling an undefined function, supplying the wrong number of arguments, or passing mismatched types raises a runtime error (catchable with `ON ERROR PROCname`).
- If the definition does not end with a leading `=` return line, calling the function is invalid.

---

**See also:**  
\ref DEF "DEF"
\ref PROC "PROC"
\ref ENDPROC "ENDPROC"
[`Parameter types`](https://github.com/brainboxdotcc/retro-rocket/wiki/Parameter-types)
