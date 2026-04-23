\page multidim Multidimensional Array Library

```BASIC
LIBRARY LIB$ + "/multidim"
```

The multidim library provides a way to work with two, three and four dimensional arrays using Retro Rocket’s native single-dimensional arrays.
It transparently handles indexing and storage layout, allowing access via coordinates rather than manual stride calculations.

The library internally stores dimension metadata alongside arrays and uses reflection to access elements.

The following publicly documented procedures and functions are available via this library.

---

### PROCdim2(name$, x, y)

Create a two-dimensional integer array.

### PROCdim2$(name$, x, y)

Create a two-dimensional string array.

### PROCdim2#(name$, x, y)

Create a two-dimensional real array.

---

### PROCredim2(name$, x, y)

Resize a two-dimensional integer array.

### PROCredim2$(name$, x, y)

Resize a two-dimensional string array.

### PROCredim2#(name$, x, y)

Resize a two-dimensional real array.

---

### FNmulti_get2(name$, x, y)

Get a value from a two-dimensional integer array.

### FNmulti_get2$(name$, x, y)

Get a value from a two-dimensional string array.

### FNmulti_get2#(name$, x, y)

Get a value from a two-dimensional real array.

---

### PROCmulti_set2(name$, x, y, value)

Set a value in a two-dimensional integer array.

### PROCmulti_set2$(name$, x, y, value$)

Set a value in a two-dimensional string array.

### PROCmulti_set2#(name$, x, y, value#)

Set a value in a two-dimensional real array.

---

### PROCdim3(name$, x, y, z)

Create a three-dimensional integer array.

### PROCdim3$(name$, x, y, z)

Create a three-dimensional string array.

### PROCdim3#(name$, x, y, z)

Create a three-dimensional real array.

---

### PROCredim3(name$, x, y, z)

Resize a three-dimensional integer array.

### PROCredim3$(name$, x, y, z)

Resize a three-dimensional string array.

### PROCredim3#(name$, x, y, z)

Resize a three-dimensional real array.

---

### FNmulti_get3(name$, x, y, z)

Get a value from a three-dimensional integer array.

### FNmulti_get3$(name$, x, y, z)

Get a value from a three-dimensional string array.

### FNmulti_get3#(name$, x, y, z)

Get a value from a three-dimensional real array.

---

### PROCmulti_set3(name$, x, y, z, value)

Set a value in a three-dimensional integer array.

### PROCmulti_set3$(name$, x, y, z, value$)

Set a value in a three-dimensional string array.

### PROCmulti_set3#(name$, x, y, z, value#)

Set a value in a three-dimensional real array.

---

### PROCdim4(name$, x, y, z, w)

Create a four-dimensional integer array.

### PROCdim4$(name$, x, y, z, w)

Create a four-dimensional string array.

### PROCdim4#(name$, x, y, z, w)

Create a four-dimensional real array.

---

### PROCredim4(name$, x, y, z, w)

Resize a four-dimensional integer array.

### PROCredim4$(name$, x, y, z, w)

Resize a four-dimensional string array.

### PROCredim4#(name$, x, y, z, w)

Resize a four-dimensional real array.

---

### FNmulti_get4(name$, x, y, z, w)

Get a value from a four-dimensional integer array.

### FNmulti_get4$(name$, x, y, z, w)

Get a value from a four-dimensional string array.

### FNmulti_get4#(name$, x, y, z, w)

Get a value from a four-dimensional real array.

---

### PROCmulti_set4(name$, x, y, z, w, value)

Set a value in a four-dimensional integer array.

### PROCmulti_set4$(name$, x, y, z, w, value$)

Set a value in a four-dimensional string array.

### PROCmulti_set4#(name$, x, y, z, w, value#)

Set a value in a four-dimensional real array.

---

### Example

```BASIC
PROCdim2("screen", 80, 25)
PROCmulti_set2("screen", 10, 5, 42)
PRINT FNmulti_get2("screen", 10, 5)

PROCdim2$("names", 10, 10)
PROCmulti_set2$("names", 3, 4, "Craig")
PRINT FNmulti_get2$("names", 3, 4)

PROCdim3#("points", 8, 8, 8)
PROCmulti_set3#("points", 1, 2, 3, 3.14159)
PRINT FNmulti_get3#("points", 1, 2, 3)
```

Coordinates are zero-based. Values outside the defined dimensions will result in access to unintended elements, or errors being thrown.
