\page DECIBELS DECIBELS Function

```basic
DECIBELS(integer-expression)
```

Converts a **decibel (dB) value** into a volume level suitable for `SOUND VOLUME`.
The return value is always in the range **0–255**, where:

* `0` means completely silent.
* `255` means full volume (unity gain).

---

### What are decibels?

Decibels are a way of describing **relative loudness**.
They don’t measure sound directly, but how much **quieter or louder** something is compared to a reference point:

* `0 dB` means “no change” - the sound plays at its original level.
* Negative values (e.g. `-6 dB`, `-12 dB`) make the sound quieter.
* The further the number is below zero, the quieter the result.
* Positive values are not useful here. Anything above `0 dB` is clamped to maximum.

For example, `-6 dB` is a common setting to “turn something down a bit” (50% loudness in Retro Rocket) without muting it.

---

### Behaviour in Retro Rocket

* `DECIBELS(0)` → 255 (full volume).
* `DECIBELS(-6)` → about half volume (128).
* `DECIBELS(-12)` → quieter still (around 64).
* `DECIBELS(-60)` → 0 (silent).

Values above 0 are treated as 0 dB.
Values below –60 are treated as silence.

This gives a simple but natural way to think about volume, since each **–6 dB step roughly halves the loudness**.

---

### Notes

* Use `DECIBELS()` when you want to set volume in familiar dB terms instead of raw 0–255 values.
* The mapping is not linear: small changes near 0 dB make a noticeable difference, while changes at very low levels fade smoothly towards silence.
* Internally this uses a fast lookup table, so performance is constant time.

---

### Examples

```basic
gain = DECIBELS(-6)
SOUND VOLUME music, gain

gain = DECIBELS(-20)
SOUND VOLUME effects, gain
```

---

**See also**
\ref SOUND "SOUND" VOLUME · \ref STREAM "STREAM"
