\page MAPHAS MAPHAS Function
```basic
X = MAPHAS(handle, key$)
````

Returns whether a key exists in a MAP.

---

### How to read it

* Looks up `key$` in the MAP identified by `handle`
* Returns `1` if the key exists, otherwise `0`

---

### Examples

```basic
M = MAP

MAPSET M, "score", 100

PRINT MAPHAS(M, "score")
PRINT MAPHAS(M, "missing")
```

Output:

```
1
0
```

---

### Notes

* The MAP must be valid
* Does not check the type of the value
* Safe to use before `MAPGET`, `MAPGETR`, or `MAPGET$` to avoid errors
* Errors are raised for:

  * invalid handle

**See also:**
\ref MAPGET "MAPGET" · \ref MAPSET "MAPSET" · \ref MAPKEYS "MAPKEYS" · \ref MAPFREE "MAPFREE"
