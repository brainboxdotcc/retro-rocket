\page DECOMPRESS DECOMPRESS Function

```basic id="w4kp6f"
DECOMPRESS(input, input_len, output, output_max_len)
```

Decompresses a gzip-compressed memory buffer.

Decompressed data is written to the output buffer and the function returns the decompressed size in bytes.

Returns zero on error.

---

### Examples

```basic id="m7qz6a"
REM Decompress a gzip-compressed memory buffer

INPUT_BUFFER = MEMALLOC(1024)
OUTPUT_BUFFER = MEMALLOC(8192)

REM Fill compressed input buffer here

SIZE = DECOMPRESS(INPUT_BUFFER, 1024, OUTPUT_BUFFER, 8192)

PRINT "Decompressed size: "; SIZE
```

```basic id="j0x2vq"
REM Load and decompress a gzip file

F = OPENIN("archive.gz")

COMPRESSED_SIZE = FILESIZE("archive.gz")

COMPRESSED = MEMALLOC(COMPRESSED_SIZE)
OUTPUT = MEMALLOC(65536)

BINREAD F, COMPRESSED, COMPRESSED_SIZE
CLOSE F

SIZE = DECOMPRESS(COMPRESSED, COMPRESSED_SIZE, OUTPUT, 65536)

PRINT "Decompressed "; COMPRESSED_SIZE; " bytes into "; SIZE
```

---

### Notes

* `input` is the compressed source memory buffer.
* `input_len` is the compressed input size in bytes.
* `output` is the destination memory buffer.
* `output_max_len` is the maximum writable size of the output buffer.
* The output buffer must be large enough to hold the decompressed data.
* Returns the decompressed size in bytes.
* Returns zero if decompression fails.

---

### Errors

* Decompression failed
* Output buffer too small

---

**See also:**
\ref COMPRESS "COMPRESS" · \ref MEMALLOC "MEMALLOC" · \ref MEMRELEASE "MEMRELEASE" · \ref BINREAD "BINREAD" · \ref BINWRITE "BINWRITE"

