\page SECSTR SECSTR$ Function

```basic
SECSTR$(length, alphabet$)
```

Returns a **cryptographically secure random string** of the specified length.

Each character is selected independently from the supplied `alphabet$`, using a uniform distribution.

The generator uses a **CTR-DRBG (counter-mode deterministic random bit generator)** provided by mbedTLS, seeded from entropy gathered by the kernel.

---

### Examples

```basic
REM Generate a 16-character random string using letters and digits
PRINT SECSTR$(16, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz")
```

```basic
REM Generate an 8-digit numeric code
PRINT SECSTR$(8, "0123456789")
```

```basic
REM Generate a random hexadecimal string
PRINT SECSTR$(32, "0123456789abcdef")
```

```basic
REM Use default alphabet (letters and digits)
PRINT SECSTR$(12, "")
```

---

### Notes

* The result is a **string** of exactly `length` characters.
* Each character is chosen with a **uniform distribution** from the provided alphabet.
  * Rejection sampling is used internally to eliminate modulo bias.
* If `alphabet$` is empty, a default alphabet of uppercase letters, lowercase letters, and digits is used.
* Suitable for:
  * tokens
  * passwords
  * identifiers
  * session keys (text form)
* Slower than non-secure alternatives due to cryptographic guarantees.
* May fail if the secure random generator is unavailable or not yet initialised.
* The caller is responsible for choosing an appropriate alphabet for the intended use.

---

**See also:**
\ref SECRND "SECRND" · \ref RND "RND"