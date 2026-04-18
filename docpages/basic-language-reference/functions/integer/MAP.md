\page MAP MAP Function

```basic
H = MAP
```

Creates a new MAP and returns its handle.

---

### How to read it

* Allocates a new empty MAP
* Returns a handle used by MAP operations

---

### Examples

```basic
M = MAP

MAPSET M, "score", 100

PRINT MAPGET(M, "score")
```

This example produces `100`.

---

### Notes

* The returned handle identifies the MAP
* A handle of `0` indicates failure
* MAPs must be freed using `MAPFREE` when no longer needed
* Using an invalid handle results in **"Invalid MAP"**
* Memory allocation failure results in **"Out of memory"**

---

### Typical usage

```basic
M = MAP

MAPSET M, "name", "PLAYER"
MAPSET M, "score", 0

PRINT MAPGET$(M, "name"), MAPGET(M, "score")

MAPFREE M
```

---

**See also:**
\ref MAPSET "MAPSET" · \ref MAPGET "MAPGET" · \ref MAPHAS "MAPHAS" · \ref MAPFREE "MAPFREE"
