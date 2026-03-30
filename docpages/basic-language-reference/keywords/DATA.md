\page DATA DATA Keyword

```basic
DATA value, value, value
```

Defines a sequence of constant values embedded in the program.

Values may be:

* integers
* real numbers
* strings (in double quotes)

Values are stored at program load time and are not re-parsed during execution.

Multiple `DATA` statements are concatenated into a single continuous data stream.

---

##### Example

```basic
DATA 10, 20
DATA 1.5
DATA "ok"
```

---

##### Behaviour

* Values are stored in the order they appear across all `DATA` statements.
* The data stream is shared across the entire program.
* `DATA` statements are not executed; they are processed during program load.
* Values are consumed sequentially by data-reading functions.
* Attempting to read beyond the available data results in a runtime error.
* Values are strongly typed; no implicit conversion occurs during reading.

---

##### See also

* \ref DATAREAD "DATAREAD"
* \ref DATAREADR "DATAREADR"
* \ref DATAREADS "DATAREAD$"
* \ref RESTORE "RESTORE"
* \ref DATASET "DATASET"
