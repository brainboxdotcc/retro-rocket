\page STRINGTOBUFFER STRINGTOBUFFER Function

```BASIC
n = STRINGTOBUFFER(string$, address, maxlen)
```

The `STRINGTOBUFFER` function decodes a BASIC string containing escaped binary data into a raw memory buffer.

It is primarily used to convert strings received from binary-safe sources (e.g. `INSOCKET$`) into a form suitable for low-level operations such as `SOCKBINWRITE`, file I/O, or direct memory processing.

The function returns the number of bytes written to the destination buffer.

---

## Parameters

* `string$`
  Source string. May contain escaped binary data.

* `address`
  Destination memory address to write decoded bytes into.

* `maxlen`
  Maximum number of bytes to write.

---

## Return value

Returns the number of bytes written to the buffer.

Returns `0` if:

* the input string is empty
* `maxlen` is `0`
* an error occurs

---

## Behaviour

* The function scans the input string and decodes escape sequences.
* Output is written sequentially into the buffer.
* Decoding stops when:

  * the end of the string is reached, or
  * `maxlen` bytes have been written

---

## Encoding format

Strings may contain escaped binary data using the following scheme:

* `CHR$(255), CHR$(1)` → logical byte `0`
* `CHR$(255), CHR$(2)` → logical byte `255`

All other bytes are copied as-is.

---

## Errors

* Invalid memory address → error
* Negative `maxlen` → error

---

## Example

```BASIC

body$ = INSOCKET$(fd, 1024)

buf = MEMALLOC(1024)

n = STRINGTOBUFFER(body$, buf, 1024)

PRINT "Bytes decoded: "; n
```

This reads data from a socket into a string, decodes it into a raw buffer, and prints the number of bytes written.

---

## Notes

* The source string is not modified.
* The destination buffer must be valid and writable.
* Escape sequences are processed transparently; invalid or partial sequences may produce undefined results.
* Intended for binary data handling; not required for normal text strings.
