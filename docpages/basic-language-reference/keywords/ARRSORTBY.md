\page ARRSORTBY ARRSORTBY Keyword

```basic
ARRSORTBY index-array,key-array[,descending]
```

Sorts `index-array` **in place**, ordering its values according to the corresponding values in `key-array`.

* `index-array` must be an **integer array**
* `key-array` may be:

  * integer array
  * real array
  * string array
* Sorting is **ascending by default**
* If `descending` is non-zero, sorting is **descending**

The values in `key-array` are **not modified**.

---

##### Examples

**Sort indices by real key (depth sort)**

```basic
DIM DEPTH#,6
DIM IDX,6

FOR I = 0 TO 5
    DEPTH#(I) = RND(1)
    IDX(I) = I
NEXT

ARRSORTBY IDX,DEPTH#,TRUE

FOR I = 0 TO 5
    PRINT IDX(I), DEPTH#(IDX(I))
NEXT
```

---

**Sort indices by integer key**

```basic
DIM VALUES,5
DIM ORDER,5

VALUES(0)=10
VALUES(1)=3
VALUES(2)=7
VALUES(3)=1
VALUES(4)=5

FOR I = 0 TO 4
    ORDER(I) = I
NEXT

ARRSORTBY ORDER,VALUES

FOR I = 0 TO 4
    PRINT ORDER(I), VALUES(ORDER(I))
NEXT
```

---

**Sort indices by string key**

```basic
DIM NAMES$,4
DIM IDX,4

NAMES$(0)="BETA"
NAMES$(1)="ALPHA"
NAMES$(2)="DELTA"
NAMES$(3)="GAMMA"

FOR I = 0 TO 3
    IDX(I) = I
NEXT

ARRSORTBY IDX,NAMES$

FOR I = 0 TO 3
    PRINT IDX(I), NAMES$(IDX(I))
NEXT
```

---

##### Notes

* Only `index-array` is reordered; `key-array` remains unchanged
* Each value in `index-array` is treated as an **index into `key-array`**
* All values in `index-array` must be within the bounds of `key-array`
* `index-array` and `key-array` must have the **same length**
* Arrays of length `0` or `1` are unchanged
* Sorting uses the type of `key-array`:

  * integer keys → numeric comparison
  * real keys → numeric comparison
  * string keys → lexicographic comparison
* String keys treat `NULL` entries as empty strings
* The relative order of equal keys is **not guaranteed to be preserved**

---

##### Typical usage

```basic
FOR F = 0 TO FACE_COUNT-1
    DEPTH#(F) = FNface_depth(F)
    IDX(F) = F
NEXT

ARRSORTBY IDX,DEPTH#,TRUE

FOR I = 0 TO FACE_COUNT-1
    PROCdraw_face(IDX(I))
NEXT
```

---

**See also:**
\ref ARRSORT "ARRSORT"
\ref DIM "DIM"
\ref REDIM "REDIM"
\ref type-array
