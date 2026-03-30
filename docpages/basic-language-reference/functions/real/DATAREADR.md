\page DATAREADR DATAREADR Function

```basic
value = DATAREADR
```

Reads the next real (floating-point) value from the `DATA` stream.

Each call returns the next value and advances the read position.

---

##### Example

```basic
DATA 1.5, 2.25, 3.75

PRINT DATAREADR
PRINT DATAREADR
PRINT DATAREADR
```

This prints:

```basic
1.5
2.25
3.75
```

---

##### Behaviour

* Returns the next value from the `DATA` stream.
* The value must be a real number.
* The read position advances after each call.
* Values are read in the order they appear in `DATA`.

---

##### Errors

* Reading past the end of available data produces an error.
* If the next value is not a real number, a runtime error is raised.

---

##### See also

* \ref DATA "DATA"
* \ref DATAREAD "DATAREAD"
* \ref DATAREADS "DATAREAD$"
* \ref RESTORE "RESTORE"
* \ref DATASET "DATASET"
