\page TOBASE64S TOBASE64$ Function

```BASIC
string$ = TOBASE64$(address, length)
```

The `TOBASE64$` function encodes binary data from a memory buffer into a Base64 ASCII string.

This is useful for safely storing or transmitting binary data in text form, such as network payloads, file contents, or encoded assets.

---

## Parameters

* `address` Source memory address containing the binary data.
* `length` Number of bytes to encode.

---

## Return value

Returns a Base64 encoded string. Returns an empty string if `length` is `0` or an error occurs

---

## Example

```BASIC
buf = MEMALLOC(10)

FOR i = 0 TO 9
    POKE buf + i, i
NEXT

b64$ = TOBASE64$(buf, 10)

PRINT b64$
```

This encodes 10 bytes from memory into a Base64 string.

---

## Notes

* The output is standard RFC 4648 Base64.
* The returned string is always ASCII text.
* Binary data can be restored using `FROMBASE64`.
* The output string is larger than the source data.
