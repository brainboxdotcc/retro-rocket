\page DIM DIM Keyword
```basic
DIM variable-name,integer-expression
```
Declares an **array** whose **element type** is determined by `variable-name` (integer, real, or string).  
Arrays are **zero-indexed** and **range-checked**: the lowest valid index is `0` and the highest is `integer-expression - 1`.

---

##### Examples

**Integer array**
```basic
DIM N,5
FOR I = 0 TO 4
    N(I) = I * I
NEXT
PRINT N(2)
```

**String array**
```basic
DIM NAMES$,3
NAMES$(0) = "ALPHA"
NAMES$(1) = "BETA"
NAMES$(2) = "GAMMA"
PRINT NAMES$(1)
```

**Real array**
```basic
DIM VALUES#,4
VALUES#(0) = 0.5
VALUES#(1) = 1.5
```

---

##### Notes
- The **element type** follows the variable’s type: e.g. `A` → integer array, `A#` → real array, `S$` → string array.
- All indexing is **0-based** (`0 .. size-1`). Access outside this range raises a runtime error (catchable with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR)).
- To change the size of an existing array, use \ref REDIM "REDIM".

**See also:** [`Array`](https://github.com/brainboxdotcc/retro-rocket/wiki/Array), [`Variable Types`](https://github.com/brainboxdotcc/retro-rocket/wiki/Variable-Types), \ref REDIM "REDIM"
