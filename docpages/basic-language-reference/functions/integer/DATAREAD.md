\page DATAREAD DATAREAD Function

```basic
value = DATAREAD
```

Reads the next integer value from the `DATA` stream.

Each call returns the next value and advances the read position.

---

##### Example

```basic
DATA 10, 20, 30

PRINT DATAREAD
PRINT DATAREAD
PRINT DATAREAD
```

This prints:

```basic
10
20
30
```

---

##### Behaviour

* Returns the next value from the `DATA` stream.
* The value must be an integer.
* The read position advances after each call.
* Values are read in the order they appear in `DATA`.

---

##### Errors

* Reading past the end of available data produces an error.
* If the next value is not an integer, a runtime error is raised.

---

##### See also

* \ref DATA "DATA"
* \ref DATAREADR "DATAREADR"
* \ref DATAREADS "DATAREAD$"
* \ref RESTORE "RESTORE"
* \ref DATASET "DATASET"
