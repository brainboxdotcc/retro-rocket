\page MAPGETR MAPGETR Function

```basic
X# = MAPGETR(handle, key$)
```

Returns the **real value** stored at `key$` in a MAP.

---

### How to read it

* Looks up `key$` in the MAP identified by `handle`
* Returns the stored value as a real number

---

### Examples

```basic
M = MAP

MAPSET M, "value", 3.5

PRINT MAPGETR(M, "value")
```

This example produces `3.5`.

---

### Integer values

```basic
MAPSET M, "count", 10

PRINT MAPGETR(M, "count")
```

This example produces `10`.

---

### Missing key

```basic
PRINT MAPGETR(M, "unknown")
```

Produces:

```
No such MAP key 'unknown'
```

---

### Type mismatch

```basic
MAPSET M, "name", "ALPHA"

PRINT MAPGETR(M, "name")
```

Produces:

```
MAP key 'name' is not REAL
```

---

### Notes

* The MAP must be valid
* The key must exist
* Accepts both:

  * real values
  * integer values (automatically converted)
* String values are not valid
* Errors are raised for:

  * invalid handle
  * missing key
  * type mismatch

**See also:**
\ref MAPGET "MAPGET" · \ref MAPGETS "MAPGET$" · \ref MAPSET "MAPSET" · \ref MAPHAS "MAPHAS"
