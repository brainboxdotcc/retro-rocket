\page BUFFERTOSTRINGS BUFFERTOSTRING$ Function

```BASIC
string$ = BUFFERTOSTRING$(address, length)
```

The `BUFFERTOSTRING$` function converts a raw memory buffer into a BASIC string, escaping any bytes that are not representable in normal strings.

It is primarily used to bring binary data (e.g. from `SOCKBINREAD`, files, or memory processing) into BASIC string form so it can be stored, passed around, or processed further.

---

## Parameters

* `address`
  Source memory address to read bytes from.

* `length`
  Number of bytes to read.

---

## Return value

Returns a BASIC string containing the encoded data.

Returns an empty string if:

* `length` is `0`
* an error occurs

---

## Behaviour

* Reads `length` bytes from memory starting at `address`
* Encodes binary data into a safe string representation
* Returns the resulting string

---

## Encoding format

The following escape scheme is used:

* logical byte `0` → `CHR$(255), CHR$(1)`
* logical byte `255` → `CHR$(255), CHR$(2)`

All other bytes are copied as-is.

This ensures the resulting string contains no embedded null bytes.

---

## Errors

* Invalid memory address → error
* Negative `length` → error
* Encoded result exceeds `MAX_STRINGLEN` → error

---

## Example

```BASIC
buf = MEMALLOC(1024)

SOCKBINREAD fd, buf, 1024

data$ = BUFFERTOSTRING$(buf, 1024)

PRINT "Received encoded string of length "; LEN(data$)
```

This reads raw binary data into a buffer, converts it into a BASIC string, and prints its length.

---

## Notes

* The returned string is safe for storage and transmission within BASIC.
* The string may be longer than the input buffer due to escaping.
* The original binary data can be recovered using `STRINGTOBUFFER`.
* This function does not interpret the data; it only encodes it.
* Intended for binary data handling; not required for normal text strings.
