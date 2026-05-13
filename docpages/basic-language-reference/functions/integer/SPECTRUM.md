\page SPECTRUM SPECTRUM Function

```basic
SPECTRUM(integer-expression)
```

Returns the current level of a frequency band from the Retro Rocket audio spectrum analyser.

The analyser continuously monitors mixed audio output and splits it into 32 frequency bands using a Fast Fourier Transform (FFT). `SPECTRUM()` allows BASIC programs to read these levels in real time to create visualisers, audio-reactive effects, VU meters, and similar displays.

The return value is always in the range:

* `0` = no activity in the band
* Approximately `1023` = maximum detected energy

---

### Frequency bands

Retro Rocket divides the audio spectrum into 32 bands meaning valid band numbers are in the range 0-31:

* Lower band numbers represent bass and low frequencies.
* Higher band numbers represent treble and high frequencies.

For example:

* `SPECTRUM(0)` may respond strongly to kick drums and basslines.
* Mid-range bands respond to vocals, guitars, and synths.
* High bands respond to cymbals, hiss, and sharp transients.

The exact frequencies represented by each band depend on the analyser configuration and FFT size.

---

### Behaviour in Retro Rocket

* `SPECTRUM(0)` returns the current level of the lowest frequency band.
* `SPECTRUM(31)` returns the current level of the highest frequency band.
* Values automatically decay over time when frequencies disappear.
* Bands update continuously while audio is playing.

If no audio is active, all bands gradually return to zero.

---

### Notes

* The analyser monitors final mixed audio output, not individual streams.
* Values are already scaled for visualisation and are suitable for direct use in graphics code.
* Frequency energy is not evenly distributed in music. Bass-heavy material naturally produces stronger low-frequency bands.
* Reading `SPECTRUM()` is inexpensive and suitable for use every frame in animation loops.

---

### Examples

Simple bar graph:

```basic
FOR B = 0 TO 31
	H = SPECTRUM(B)
	LINE B * 10, 200, B * 10, 200 - H
NEXT
```

Simple colour-reactive effect:

```basic
BASS = SPECTRUM(0)

GCOL RGB(BASS, 0, 255 - BASS)
CLS
```

---

**See also**
\ref SOUND "SOUND" · \ref STREAM "STREAM" · \ref DECIBELS "DECIBELS"
