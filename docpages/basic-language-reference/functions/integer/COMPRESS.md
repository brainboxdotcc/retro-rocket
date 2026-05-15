\page COMPRESS COMPRESS Function

```basic
COMPRESS(input, input_len, output, output_max_len, level)
```

Compresses a memory buffer using gzip compression.

Compressed data is written to the output buffer and the function returns the compressed size in bytes.

Returns zero on error.

---

### Examples

```basic
REM Compress a memory buffer

INPUT_BUFFER = MEMALLOC(1024)
OUTPUT_BUFFER = MEMALLOC(2048)

REM Fill input buffer here

SIZE = COMPRESS(INPUT_BUFFER, 1024, OUTPUT_BUFFER, 2048, 6)

PRINT "Compressed size: "; SIZE
```

```basic
REM Compress a file loaded into memory

F = OPENIN("data.bin")

SIZE = FILESIZE("data.bin")

INPUT = MEMALLOC(SIZE)
OUTPUT = MEMALLOC(SIZE * 2)

BINREAD F, INPUT, SIZE
CLOSE F

COMPRESSED = COMPRESS(INPUT, SIZE, OUTPUT, SIZE * 2, 9)

PRINT "Compressed "; SIZE; " bytes into "; COMPRESSED
```

---

### Notes

* `input` is the source memory buffer.
* `input_len` is the size of the source data in bytes.
* `output` is the destination memory buffer.
* `output_max_len` is the maximum writable size of the output buffer.
* `level` is the gzip compression level from 0 to 9.
* Higher compression levels are slower but may produce smaller output.
* The output buffer must be large enough to hold the compressed data.
* Returns the compressed size in bytes.
* Returns zero if compression fails.

---

### Errors

* Compression failed
* Output buffer too small

---

**See also:**
\ref DECOMPRESS "DECOMPRESS" · \ref MEMALLOC "MEMALLOC" · \ref MEMRELEASE "MEMRELEASE" · \ref BINREAD "BINREAD" · \ref BINWRITE "BINWRITE"

