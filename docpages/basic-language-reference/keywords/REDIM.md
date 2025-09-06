\page REDIM REDIM Keyword
```basic
REDIM variable-name,integer-expression
```

Changes the **capacity** of an existing array to the new size given by `integer-expression`.

- Arrays are **zero-indexed** and **range-checked**: valid indices are `0` to `size - 1`.
- The array’s **element type** is determined by `variable-name` and does not change (`A` integer, `A#` real, `A$` string).
- Calling `REDIM` with the **current size** performs **no operation**.


\remark `REDIM` preserves existing elements from index `0` up to `min(old_size, new_size) - 1`.
\remark When **growing**, new slots are initialised to `0` (integer or real arrays) or `""` (string arrays).
> When **shrinking**, elements at indices `new_size` to `old_size - 1` are **permanently lost**.


> `REDIM` changes the **size only**. It does not change type, and it does not create a new array name.  
> Declare the array first with [`DIM`](https://github.com/brainboxdotcc/retro-rocket/wiki/DIM).

---

### Examples

**Grow an integer array**
```basic
DIM A,3
A(0) = 10
A(1) = 20
A(2) = 30

REDIM A,5
PRINT A(0)
PRINT A(1)
PRINT A(2)
REM prints 0
PRINT A(3)
REM prints 0   
PRINT A(4)
```

**Shrink and lose tail elements**
```basic
DIM B,5
B(0) = 1
B(1) = 2
B(2) = 3
B(3) = 4
B(4) = 5

REDIM B,3
PRINT B(0)
PRINT B(1)
PRINT B(2)
REM B(3) and B(4) no longer exist
```

**Make room before inserting with PUSH**
```basic
DIM Q,3
Q(0) = 100
Q(1) = 200
Q(2) = 300

REDIM Q,4
PUSH Q,1
Q(1) = 150
```

**Strings**
```basic
DIM NAMES$,2
NAMES$(0) = "ALPHA"
NAMES$(1) = "BETA"

REDIM NAMES$,3
REM prints empty string
PRINT NAMES$(2)
```

---

### Behaviour and constraints
- The **new size** must be large enough for any indices you intend to access (`0..new_size-1`).
- After shrinking, accessing an index `>= new_size` raises a runtime error.
- Use `REDIM` together with [`PUSH`](https://github.com/brainboxdotcc/retro-rocket/wiki/PUSH) or [`POP`](https://github.com/brainboxdotcc/retro-rocket/wiki/POP) to manage contents while changing capacity.

**See also:**  
[`DIM`](https://github.com/brainboxdotcc/retro-rocket/wiki/DIM) ·
[`PUSH`](https://github.com/brainboxdotcc/retro-rocket/wiki/PUSH) ·
[`POP`](https://github.com/brainboxdotcc/retro-rocket/wiki/POP) ·
[`Array`](https://github.com/brainboxdotcc/retro-rocket/wiki/Array)
