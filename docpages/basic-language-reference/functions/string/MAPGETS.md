\page MAPGETS MAPGET$ Function
```basic
S$ = MAPGET$(handle, key$)
````

Returns the **string value** stored at `key$` in a MAP.

---

### How to read it

* Looks up `key$` in the MAP identified by `handle`
* Returns the stored value as a string

---

### Examples

```basic
M = MAP

MAPSET M, "name", "ALPHA"

PRINT MAPGET$(M, "name")
```

This example produces:

```
ALPHA
```

---

### Missing key

```basic id="r1n9yb"
PRINT MAPGET$(M, "unknown")
```

Produces:

```id="l8p2dx"
No such MAP key 'unknown'
```

---

### Type mismatch

```basic
MAPSET M, "value", 123

PRINT MAPGET$(M, "value")
```

Produces:

```
MAP key 'value' is not STRING
```

---

### Notes

* The MAP must be valid
* The key must exist
* The value must be a string
* Returned value is a copy; modifying it does not affect the MAP
* Numeric values are not automatically converted to strings
* Errors are raised for:

  * invalid handle
  * missing key
  * type mismatch

**See also:**
\ref MAPGET "MAPGET" · \ref MAPGETR "MAPGETR" · \ref MAPSET "MAPSET" · \ref MAPHAS "MAPHAS"
