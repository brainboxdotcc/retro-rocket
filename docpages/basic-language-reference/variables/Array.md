\page type-array Array Variables

Arrays in Retro Rocket BASIC are collections of variables of the same type, indexed by a **zero-based subscript** in brackets.

* String arrays end with `$` (e.g. `names$(0)`),
* Real arrays end with `#` (e.g. `values#(10)`),
* Integer arrays have no suffix (e.g. `scores(5)`).

---

### Declaring arrays

Arrays must be created with the \ref DIM "DIM" statement:

```basic
DIM names$, 100
DIM values#, 50
DIM scores, 200
```

This allocates arrays with the specified number of elements.

Arrays can be resized dynamically with \ref REDIM "REDIM":

```basic
REDIM names$, 200
```

---

### Accessing elements

Use a zero-based index to read or write individual elements:

```basic
names$(0) = "Alice"
scores(5) = 42
values#(10) = 3.14
```

---

### Assigning to all elements

Assigning to the array name without a subscript initialises **all elements**:

```basic
scores = 0
names$ = "foo"
```

This sets every element of `scores` to `0` and every element of `names$` to `"foo"`.

---

### Notes

* Arrays are always contiguous in memory.
* Indexes must be within bounds (0 to length–1) or an error occurs.
* Resizing with `REDIM` preserves existing values where possible.

---

**See also:**
\ref DIM "DIM" · \ref REDIM "REDIM" · \ref PUSH "PUSH" · \ref POP "POP"
