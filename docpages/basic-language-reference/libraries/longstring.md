\page longstring Long String Library

```BASIC
LIBRARY LIB$ + "/longstring"
```

The longstring library provides a MAP-backed representation of strings that may exceed the normal string size limit.

A long string is treated as an opaque handle and must only be accessed through the provided functions and procedures. Internally, data is stored as a sequence of chunks, allowing safe handling of arbitrarily large data without concatenation.

The following publicly documented procedures and functions are available via this library.

---

## Creation and modification

### FNnew_long_string

Create and return a new empty long string.

```BASIC
ls = FNnew_long_string
```

---

### PROClong_string_append(ls, s$)

Append a normal string `s$` to the end of the long string.

Data is stored as an additional chunk. No concatenation is performed.

---

## Inspection

### FNlong_string_len(ls)

Return the total length of the long string in characters.

This value is maintained incrementally and does not require scanning the data.

---

### FNlong_string_chunks(ls)

Return the number of chunks stored internally.

---

### FNlong_string_chunk$(ls, idx)

Return the chunk at index `idx`.

Returns an empty string if `idx` is out of range.

This function is primarily intended for streaming or debugging.

---

## Substring operations

All substring operations use **0-based indexing**.

These functions do not concatenate the full string; only the requested portion is constructed.

---

### FNlong_string_mid$(ls, start, length)

Return a substring beginning at `start` with maximum length `length`.

* `start` is 0-based
* returns an empty string if `length <= 0`
* safely handles out-of-range values

---

### FNlong_string_left$(ls, n)

Return the first `n` characters of the long string.

Equivalent to:

```BASIC
FNlong_string_mid$(ls, 0, n)
```

---

### FNlong_string_right$(ls, n)

Return the last `n` characters of the long string.

If `n` exceeds the total length, the entire string is returned.

---

### FNlong_string_substr$(ls, start, length)

Alias of `FNlong_string_mid$`.

Provided for naming consistency with other environments.

---

## Search

### FNlong_string_instr(ls, needle$)

Search for the first occurrence of `needle$` within the long string.

* returns a **1-based position** if found
* returns `0` if not found

The search is performed safely across chunk boundaries.

---

## Usage patterns

Long strings are designed for **streaming and partial access**, not full concatenation.

Typical usage:

```BASIC
ls = FNnew_long_string

PROClong_string_append(ls, "Hello ")
PROClong_string_append(ls, "world")

PRINT FNlong_string_mid$(ls, 0, 5)
```

---

### Streaming output

```BASIC
FOR i = 0 TO FNlong_string_chunks(ls) - 1
    PRINT FNlong_string_chunk$(ls, i);
NEXT
```

---

## Notes

* Long strings are internally MAP-backed and should be treated as opaque.
* Data is stored as discrete chunks; no automatic joining is performed.
* Substring operations only construct the requested portion, making them safe for large data.
* The library is designed for use with streaming inputs such as sockets, files, and protocol parsers.
* Avoid constructing full strings unless you are certain the result will not exceed normal string limits.
