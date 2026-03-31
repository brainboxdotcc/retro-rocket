\page ARRAYFIND ARRAYFIND Keyword

```basic
ARRAYFIND source-array,predicate,dest-array,count-variable
```

Searches `source-array` for elements equal to `predicate`, and returns the **indices of matching elements** in `dest-array`.

The **number of matches** is written to `count-variable`.

* `dest-array` is always an **integer array**
* `count-variable` is an **integer variable**
* Matching is **type-sensitive**:

  * integer arrays → integer comparison
  * real arrays → exact real comparison
  * string arrays → string comparison

---

##### Examples

**Integer array**

```basic
DIM N,5
N(0)=1
N(1)=2
N(2)=1
N(3)=3
N(4)=1

ARRAYFIND N,1,FOUND,COUNT

FOR I = 0 TO COUNT-1
    PRINT FOUND(I)
NEXT
```

---

**String array**

```basic
DIM NAMES$,4
NAMES$(0)="ALPHA"
NAMES$(1)="BETA"
NAMES$(2)="ALPHA"
NAMES$(3)="GAMMA"

ARRAYFIND NAMES$,"ALPHA",MATCHES,COUNT

FOR I = 0 TO COUNT-1
    PRINT MATCHES(I)
NEXT
```

---

**No matches**

```basic
DIM N,3
N(0)=10
N(1)=20
N(2)=30

ARRAYFIND N,5,FOUND,COUNT

PRINT COUNT
PRINT FOUND(0)
```

Output:

```
0
-1
```

---

##### Notes

* `dest-array` is **automatically created or resized**:

  * If it does not exist, it is created with `DIM`
  * If it exists, it is resized with `REDIM`
* The result contains **indices**, not values
* Matches are returned in **ascending index order**
* If no matches are found:

  * `count-variable` is set to `0`
  * `dest-array` is resized to length `1` and contains `-1`
* `source-array`, `dest-array`, and `count-variable` must all be **distinct**
* The type of `predicate` must match the type of `source-array`

---

##### Typical usage

```basic
SPRITEROW LEVEL,tick,ROW
ARRAYFIND ROW,RGB(255,0,0),ENEMIES,COUNT

FOR I = 0 TO COUNT-1
    PROCspawn_enemy(ENEMIES(I))
NEXT
```

---

**See also:**
\ref DIM "DIM"
\ref REDIM "REDIM"
\ref type-array
