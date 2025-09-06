\page PUSH PUSH Keyword
```basic
PUSH variable-name,integer-expression
```

Inserts a new element into an **array** at the given **index**, shifting existing elements at that index and above **up by one place** to make room.  
The inserted element is initialised to **0** (for integer and real arrays) or **""** (for string arrays).


\remark Arrays are **zero-indexed** and **range-checked**.
\remark Valid indices are `0` to `size-1`. An out-of-range index raises a runtime error.


\remark `PUSH` does **not** change the array’s size. The element that was previously at index `size-1` is **discarded**.
\remark If you want to grow the array, call [`REDIM`](https://github.com/brainboxdotcc/retro-rocket/wiki/REDIM) **first**.

---

### How it works

- Before: `A(0) .. A(size-2), A(size-1)`
- `PUSH A, i`:
  - Elements `i .. size-2` move to `i+1 .. size-1`
  - `A(i)` becomes `0` or `""`
  - The old `A(size-1)` is lost

Time cost is proportional to the number of elements moved.

---

### Common patterns

**Insert a value at a position**  
Use `PUSH` to make room, then assign into the cleared slot.
```basic
DIM A,5
A(0) = 10
A(1) = 20
A(2) = 30
A(3) = 40
A(4) = 50

PUSH A,2            REM make room at index 2
A(2) = 999          REM set the inserted element
```

**Insert at the front (index 0)**
```basic
DIM Q,4
Q(0) = 1
Q(1) = 2
Q(2) = 3
Q(3) = 4

PUSH Q,0
Q(0) = 100
```

**Insert at the end position (index size-1)**  
This clears the last slot without moving others.
```basic
DIM B,3
B(0) = 7
B(1) = 8
B(2) = 9

PUSH B,2
B(2) = 99
```

**Grow then insert without losing data**
```basic
DIM N,3
N(0) = 1
N(1) = 2
N(2) = 3

REDIM N,4           REM make space for one more element
PUSH N,1            REM shift from index 1 upward
N(1) = 20           REM write inserted value
```

**Strings**
```basic
DIM NAMES$,4
NAMES$(0) = "ALPHA"
NAMES$(1) = "BETA"
NAMES$(2) = "GAMMA"
NAMES$(3) = "DELTA"

PUSH NAMES$,2
NAMES$(2) = "OMEGA"
```

---

### Notes

- Works with **integer**, **real**, and **string** arrays; the inserted default is `0` or `""` accordingly.
- Inserting at `size-1` does not shift any values; it simply clears that last position.
- If you need to insert multiple elements, consider calling `REDIM` once to grow the array, then perform several `PUSH` operations.

**See also:**  
[`POP`](https://github.com/brainboxdotcc/retro-rocket/wiki/POP) ·
[`REDIM`](https://github.com/brainboxdotcc/retro-rocket/wiki/REDIM) ·
[`DIM`](https://github.com/brainboxdotcc/retro-rocket/wiki/DIM)
