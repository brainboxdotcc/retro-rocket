\page POP POP Keyword
```basic
POP variable-name,integer-expression
```

Removes the element at the given **index** from an **array**, shifting higher-index elements **down by one place** to fill the gap.  
The array’s **size does not change**: the final slot is cleared to **0** (integer/real arrays) or **""** (string arrays).


\remark Arrays are **zero-indexed** and **range-checked**.
\remark Valid indices are `0` to `size-1`. An out-of-range index raises a runtime error.


\remark `POP` does **not** resize the array. If you also want to shrink its capacity, combine it with [`REDIM`](https://github.com/brainboxdotcc/retro-rocket/wiki/REDIM).

---

### Examples

**Remove an integer element**
```basic
DIM A,5
A(0) = 10
A(1) = 20
A(2) = 30
A(3) = 40
A(4) = 50

POP A,2
REM A becomes: [10,20,40,50,0]
```

**Remove a string element**
```basic
DIM NAMES$,4
NAMES$(0) = "ALPHA"
NAMES$(1) = "BETA"
NAMES$(2) = "GAMMA"
NAMES$(3) = "DELTA"

POP NAMES$,1
REM NAMES$ becomes: ["ALPHA","GAMMA","DELTA",""]
```

**Shrink capacity after a POP (optional)**
```basic
DIM Q,4
Q(0) = 1
Q(1) = 2
Q(2) = 3
Q(3) = 4

POP Q,0
REDIM Q,3
```

---

### Behaviour
- **Index at last element**: nothing is shifted; the last slot is simply cleared.
- **Time cost**: proportional to the number of elements moved (those after the removed index).
- Works with **integer**, **real**, and **string** arrays; the clearing value is `0`, `0`, or `""` respectively.

**See also:**  
[`DIM`](https://github.com/brainboxdotcc/retro-rocket/wiki/DIM) ·
[`REDIM`](https://github.com/brainboxdotcc/retro-rocket/wiki/REDIM) ·
[`PUSH`](https://github.com/brainboxdotcc/retro-rocket/wiki/PUSH)
