\page MAPSET MAPSET Keyword

```basic
MAPSET handle, key$, value
````

Sets or replaces a key/value pair in a MAP.

* `handle` must be a valid MAP handle
* `key$` is a string key
* `value` may be:

  * integer
  * real
  * string

---

##### Examples

**Set integer value**

```basic
M = MAP
MAPSET M, "score", 100

PRINT MAPGET(M, "score")
```

Output:

```
100
```

---

**Set string value**

```basic
MAPSET M, "name", "ALPHA"

PRINT MAPGET$(M, "name")
```

---

**Replace existing value**

```basic
MAPSET M, "score", 100
MAPSET M, "score", 250

PRINT MAPGET(M, "score")
```

Output:

```
250
```

---

**Integer ↔ real compatibility**

```basic
MAPSET M, "value", 10
MAPSET M, "value", 3.5

PRINT MAPGETR(M, "value")
```

---

##### Notes

* Keys are **case-sensitive**
* Keys are stored as copies; modifying the original string has no effect
* Values are **deep-copied** into the MAP
* Setting an existing key:

  * replaces the previous value
  * frees the previous stored value
* Type rules:

  * integer and real are interchangeable
  * string is **not interchangeable** with numeric types
  * other type changes result in **"Type mismatch for MAP key"**
* Using an invalid handle results in **"Invalid MAP"**
* Memory allocation failure results in **"Out of memory"**

---

##### Typical usage

```basic
M = MAP

MAPSET M, "name", "PLAYER"
MAPSET M, "score", 0

MAPSET M, "score", MAPGET(M, "score") + 10

PRINT MAPGET$(M, "name"), MAPGET(M, "score")
```

---

**See also:**
\ref MAP "MAP"
\ref MAPGET "MAPGET"
\ref MAPGET$ "MAPGET$"
\ref MAPGETR "MAPGETR"
\ref MAPHAS "MAPHAS"
\ref MAPFREE "MAPFREE"
\ref MAPKEYS "MAPKEYS"
