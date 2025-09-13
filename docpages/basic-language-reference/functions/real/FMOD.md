\page FMOD FMOD Function

```basic
FMOD(real-x, real-y)
```

Returns the **floating-point remainder** of `x ÷ y`.
This is the value left over after dividing `x` by `y`, keeping the **sign of the dividend** (`x`).

---

### Examples

```basic
PRINT FMOD(5.3, 2)
```

Produces `1.3`.

```basic
PRINT FMOD(-5.3, 2)
```

Produces `-1.3`.

```basic
REM Use FMOD to wrap angles into 0–360 degrees
angle# = 725
PRINT FMOD(angle#, 360)
```

Produces `5`.

---

### Notes

* `y` must not be `0` — this causes an error.
* The result has the same sign as the dividend (`x`).
* Differs from integer modulus (`%`) because it works with reals and preserves fractional parts.
* Useful for periodic functions, angle wrapping, and floating-point arithmetic.

---

**See also:**
\ref ROUND "ROUND"
