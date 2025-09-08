\page LET LET Keyword
```basic
LET variable-name = expression
```
```basic
variable-name = expression
```

Assigns the value of `expression` to `variable-name`.  
`LET` is **optional** - you may write assignments **with or without** the keyword.
A variable is created on first assignment and its **type** follows its name:
- no suffix  -> integer
- name ending with `#` -> real
- name ending with `$` -> string

---

### Examples

Integer assignment (with and without `LET`)
```basic
LET N = 10
M = 20
PRINT N + M
```

Real and string assignments
```basic
X# = 3.14159
NAME$ = "ALICE"
PRINT X#, NAME$
```

Array element assignment
```basic
DIM A,5
A(0) = 42
A(1) = A(0) + 8
PRINT A(1)
```

Using an expression on the right-hand side
```basic
TOTAL = PRICE * QUANTITY
```

---

### Notes
- `LET` does not change scope. It is simply an optional keyword for assignment.
- The **left-hand side** must be a variable (or array element), not an expression.
- Strings are written with double quotes. To include a double quote inside a string you can build it with `CHR$(34)` if needed.
- For arrays, ensure you have declared the array size with \ref DIM "DIM" before assigning elements.
- `GLOBAL` is unrelated to `LET`: use \ref GLOBAL "GLOBAL" when you want the value copied into child programs started with `CHAIN`.

**See also:**  
[`Variable Naming`](https://github.com/brainboxdotcc/retro-rocket/wiki/Variable-Naming) ·
[`Variable Types`](https://github.com/brainboxdotcc/retro-rocket/wiki/Variable-Types) ·
\ref DIM "DIM" ·
\ref GLOBAL "GLOBAL"
