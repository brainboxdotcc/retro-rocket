\page FROMBASE64 FROMBASE64 Function

```BASIC
address = FROMBASE64(base64$, buffer, length)
```

The `FROMBASE64` function decodes a Base64 string into a raw memory buffer.

This is useful for restoring binary data previously encoded with `TOBASE64$`.

---

## Parameters

* `base64$` Base64 encoded ASCII string.
* `buffer` Destination memory address for decoded data.
* `length` Maximum size of the destination buffer.

---

## Return value

Returns the destination buffer address on success. Returns `0` if decoding fails.

---

## Example

```BASIC
b64$ = "AAECAwQFBgcICQ=="

buf = MEMALLOC(32)

ptr = FROMBASE64(b64$, buf, 32)

PRINT PEEK(ptr)
PRINT PEEK(ptr + 1)
PRINT PEEK(ptr + 2)
```

This decodes Base64 data into a memory buffer and reads back the first few bytes.

---

## Notes

* The decoded binary size may be smaller than the supplied buffer.
* The destination buffer must already exist.
* Intended for binary data handling rather than normal text strings.
* Data encoded with `TOBASE64$` can be restored losslessly with this function.