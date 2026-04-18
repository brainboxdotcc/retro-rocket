\page MAPGET MAPGET Function
```basic
X = MAPGET(handle, key$)
```

Returns the **integer value** stored at `key$` in a MAP.

---

### How to read it

* Looks up `key$` in the MAP identified by `handle`
* Returns the stored value if it is an integer

---

### Examples

```basic
M = MAP

MAPSET M, "score", 100

PRINT MAPGET(M, "score")
```

This example produces `100`.

---

### Missing key

```basic
PRINT MAPGET(M, "unknown")
```

Produces:

```
No such MAP key 'unknown'
```

---

### Type mismatch

```basic
MAPSET M, "name", "ALPHA"

PRINT MAPGET(M, "name")
```

Produces:

```
MAP key 'name' is not INTEGER
```

---

### Notes

* The MAP must be valid
* The key must exist
* The value must be an integer
* Real values should be accessed using `MAPGETR`
* String values should be accessed using `MAPGET$`
* Errors are raised for:

  * invalid handle
  * missing key
  * type mismatch

**See also:**
\ref MAPSET "MAPSET" · \ref MAPGETR "MAPGETR" · \ref MAPGETS "MAPGET$" · \ref MAPHAS "MAPHAS"
