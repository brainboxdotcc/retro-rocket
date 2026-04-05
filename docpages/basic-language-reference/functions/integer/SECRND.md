\page SECRND SECRND Function

```basic
SECRND(min, max)
```

Returns a **cryptographically secure random integer** between `min` and `max` (inclusive).

Both parameters must be integers.

The generator uses a **CTR-DRBG (counter-mode deterministic random bit generator)** provided by mbedTLS, seeded from entropy gathered by the kernel.

---

### Examples

```basic
REM Secure coin toss (1 or 2)
PRINT SECRND(1, 2)
```

```basic
REM Secure dice roll
PRINT SECRND(1, 6)
```

```basic
REM Generate a secure random number between 1000 and 9999
PRINT SECRND(1000, 9999)
```

```basic
REM Generate 5 secure random values
FOR i = 1 TO 5
    PRINT SECRND(1, 100)
NEXT
```

---

### Notes

* The result is an **integer** within the inclusive range `[min, max]`.
* The distribution is **uniform** — every value in the range is equally likely.
  * Rejection sampling is used internally to eliminate modulo bias.
* If `min > max`, the range is handled automatically (values are swapped internally).
* Suitable for:
  * tokens
  * authentication codes
  * security-sensitive randomness
* Slower than \ref RND "RND" due to cryptographic guarantees.
* May fail if the secure random generator is unavailable or not yet initialised.

---

**See also:**
\ref SECSTR "SECSTR$" · \ref RND "RND"