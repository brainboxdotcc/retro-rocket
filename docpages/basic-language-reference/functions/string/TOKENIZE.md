\page TOKENIZE TOKENIZE$ Function

```basic
TOKENIZE$(string-variable, string-expression)
```

Splits a string variable into tokens, using the given separator string.

* Returns the portion of the string **before** the separator.
* Modifies the original string variable so that it now contains only the **remainder** (after the separator).
* If the separator does not occur, the entire string is returned and the variable becomes empty.
* The separator can be more than one character.

---

### Examples

```basic
V$ = "ONE TWO"
A$ = TOKENIZE$(V$, " ")
B$ = TOKENIZE$(V$, " ")
PRINT A$, B$
```

Produces:

```
ONE TWO
```

and afterwards `V$` is empty.

\image html tokenize.png

```basic
REM Split a CSV line
line$ = "red,green,blue"
c1$ = TOKENIZE$(line$, ",")
c2$ = TOKENIZE$(line$, ",")
c3$ = TOKENIZE$(line$, ",")
PRINT c1$, c2$, c3$
```

Produces:

```
red green blue
```

---

### Notes

* The first parameter must be a **string variable** (not a literal), because its value is modified in-place.
* The second parameter is the **separator string**.
* Useful for parsing space-separated or delimiter-separated text.

---

**See also:**
\ref LEFT "LEFT$" · \ref RIGHT "RIGHT$" · \ref MID "MID$" · \ref REVERSE "REVERSE$"
