\page RESTORE RESTORE Keyword

```basic
RESTORE
RESTORE offset
```

Resets or repositions the current `DATA` read pointer.

With no argument, `RESTORE` resets the read position to the start of the program's data stream.

With an argument, `RESTORE` sets the read position to a specific zero-based `DATA` offset.

---

##### Example

```basic
DATA 10, 20
DATA 30

PRINT DATAREAD
PRINT DATAREAD

RESTORE

PRINT DATAREAD
```

This prints:

```basic
10
20
10
```

---

##### Example with offset

```basic
DATA 10, 20, 30

RESTORE 1
PRINT DATAREAD
PRINT DATAREAD
```

This prints:

```basic
20
30
```

---

##### Behaviour

* `RESTORE` with no argument resets the data pointer to offset `0`.
* `RESTORE N` moves the data pointer to offset `N`.
* Offsets are zero-based.
* The offset refers to the internal position within the combined `DATA` stream, not to a line number.
* An invalid offset produces a runtime error.
* `RESTORE` affects subsequent calls to \ref DATAREAD "DATAREAD", \ref DATAREADR "DATAREADR", and \ref DATAREADS "DATAREAD$".

---

##### See also

* \ref DATA "DATA"
* \ref DATASET "DATASET"
* \ref DATAREAD "DATAREAD"
* \ref DATAREADR "DATAREADR"
* \ref DATAREADS "DATAREAD$"
