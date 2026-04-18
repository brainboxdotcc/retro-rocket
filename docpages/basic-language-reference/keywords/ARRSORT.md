\page ARRSORT ARRSORT Keyword

```basic
ARRSORT array[,descending]
```

Sorts the contents of `array` **in place**.

* Sorting is **ascending by default**
* If `descending` is non-zero, sorting is **descending**
* Works with:

  * integer arrays
  * real arrays
  * string arrays

---

##### Examples

**Integer array (ascending)**

```basic
DIM N,5
N(0)=5
N(1)=2
N(2)=4
N(3)=1
N(4)=3

ARRSORT N

FOR I = 0 TO 4
    PRINT N(I)
NEXT
```

Output:

```
1
2
3
4
5
```

---

**Integer array (descending)**

```basic
ARRSORT N,TRUE
```

Output:

```
5
4
3
2
1
```

---

**String array**

```basic
DIM NAMES$,4
NAMES$(0)="BETA"
NAMES$(1)="ALPHA"
NAMES$(2)="DELTA"
NAMES$(3)="GAMMA"

ARRSORT NAMES$

FOR I = 0 TO 3
    PRINT NAMES$(I)
NEXT
```

---

**Real array**

```basic
DIM D#,4
D#(0)=3.5
D#(1)=1.2
D#(2)=4.8
D#(3)=2.0

ARRSORT D#
```

---

##### Notes

* Sorting is **in place**; the original order is replaced
* The array must already exist
* Arrays of length `0` or `1` are unchanged
* Sorting uses a **general-purpose comparison** appropriate to the array type:

  * integers → numeric comparison
  * reals → numeric comparison
  * strings → lexicographic comparison
* String sorting compares values using standard string comparison rules
* The relative order of equal elements is **not guaranteed to be preserved**

---

##### Typical usage

```basic
ARRSORT SCORES,TRUE

FOR I = 0 TO 9
    PRINT SCORES(I)
NEXT
```

---

**See also:**
\ref ARRSORTBY "ARRSORTBY"
\ref DIM "DIM"
\ref REDIM "REDIM"
\ref type-array
