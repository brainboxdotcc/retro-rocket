\page RND RND Function

```basic
RND(min, max)
```

Returns a **pseudo-random integer** between `min` and `max` (inclusive).

Both parameters must be integers.

The generator uses the **Mersenne Twister (MT19937)** algorithm, seeded from entropy provided by the operating system kernel.

\image html rnd.png

---

### Examples

```basic
REM Toss a coin (1 or 2)
PRINT RND(1, 2)
```

```basic
REM Roll a six-sided die
PRINT RND(1, 6)
```

```basic
REM Pick a random number between 50 and 5000
PRINT RND(50, 5000)
```

```basic
REM Generate 5 random values between 100 and 200
FOR i = 1 TO 5
    PRINT RND(100, 200)
NEXT
```

---

### Notes

* The result is an **integer** within the inclusive range `[min, max]`.
* The distribution is **uniform** — every value in the range is equally likely.
  * Internally, rejection sampling is used to eliminate modulo bias.
* If `min > max`, the range is handled automatically (values are swapped internally).
* Suitable for games, simulations, procedural generation, and general-purpose randomness.
* **Not cryptographically secure**:
  * MT19937 is deterministic and predictable if its internal state is known.
  * For security-sensitive uses (tokens, keys, authentication), use \ref SECRND "SECRND" instead.
* **Retro Rocket BASIC difference:**
  * BBC BASIC used a 33-bit LFSR (linear feedback shift register).
  * Retro Rocket uses MT19937, providing higher-quality statistical randomness.

---

**See also:**
\ref SECRND "SECRND" · \ref INT "INT" · \ref ABS "ABS" · \ref SECSTR "SECSTR$"