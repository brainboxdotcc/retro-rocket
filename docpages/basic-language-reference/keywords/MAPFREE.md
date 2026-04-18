\page MAPFREE MAPFREE Keyword

```basic
MAPFREE handle
```

Releases a MAP previously created with `MAP`.

After calling `MAPFREE`, the `handle` becomes **invalid** and must not be used again.

---

##### Example

```basic
M = MAP

MAPSET M, "a", 123
PRINT MAPGET(M, "a")

MAPFREE M

PRINT MAPGET(M, "a")
```

Output:

```
123
Invalid MAP
```

---

##### Notes

* Frees all memory associated with the MAP:

  * all keys
  * all stored values
  * the underlying hashmap
* The handle is removed from the internal handle table
* Using a freed or unknown handle results in **"Invalid MAP"**
* Calling `MAPFREE` does **not** clear the variable holding the handle value
* Passing `0` is always invalid

---

##### Typical usage

```basic
M = MAP

MAPSET M, "score", 100

REM ... use map ...

MAPFREE M
```

---

**See also:**
\ref MAP "MAP"
\ref MAPSET "MAPSET"
\ref MAPGET "MAPGET"
\ref MAPHAS "MAPHAS"
\ref MAPKEYS "MAPKEYS"
