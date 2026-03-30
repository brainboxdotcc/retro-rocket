\page DATASET DATASET Keyword

```basic
DATASET name
```

Defines a named position within the `DATA` stream.

The name is set to the current position in the `DATA` stream at that point in the program, so it can be used later with `RESTORE`.

---

##### Example

```basic
DATA 10, 20
DATASET numbers

DATA 30, 40

RESTORE numbers
PRINT DATAREAD
PRINT DATAREAD
```

This prints:

```basic
30
40
```

---

##### Behaviour

* `DATASET` marks a position in the `DATA` stream.
* The name becomes a variable containing that position.
* The position is zero-based.
* `DATASET` does not consume any data.
* `DATASET` statements are processed when the program is loaded, not during execution.
* The named position can be used with \ref RESTORE "RESTORE".

---

##### Notes

* The value assigned to the name is an integer.
* The name follows normal variable naming rules.
* If multiple `DATASET` statements use the same name, the last one wins.

---

##### See also

* \ref DATA "DATA"
* \ref RESTORE "RESTORE"
* \ref DATAREAD "DATAREAD"
* \ref DATAREADR "DATAREADR"
* \ref DATAREADS "DATAREAD$"
