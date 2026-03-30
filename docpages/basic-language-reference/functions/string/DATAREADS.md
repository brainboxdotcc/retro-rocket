\page DATAREADS DATAREAD$ Function

```basic
value$ = DATAREAD$
```

Reads the next string value from the `DATA` stream.

Each call returns the next value and advances the read position.

---

##### Example

```basic
DATA "hello", "world"

A$ = DATAREAD$
B$ = DATAREAD$

PRINT A$
PRINT B$
```

This prints:

```basic
hello
world
```

---

##### Behaviour

* Returns the next value from the `DATA` stream.
* The value must be a string.
* The read position advances after each call.
* Values are read in the order they appear in `DATA`.

---

##### Errors

* Reading past the end of available data produces an error.
* If the next value is not a string, a runtime error is raised.

---

##### See also

* \ref DATA "DATA"
* \ref DATAREAD "DATAREAD"
* \ref DATAREADR "DATAREADR"
* \ref RESTORE "RESTORE"
* \ref DATASET "DATASET"
