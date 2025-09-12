\page RGB RGB Function

```basic
RGB(red, green, blue)
```

Combines three integers into a single **RGB colour value**.
Each parameter must be an integer in the range **0–255**:

* `red` → intensity of the red channel
* `green` → intensity of the green channel
* `blue` → intensity of the blue channel

---

### Examples

```basic
REM Create pure red
col = RGB(255, 0, 0)
PRINT "Red = "; col
```

```basic
REM Create a shade of purple
col = RGB(128, 0, 128)
PRINT "Purple = "; col
```

```basic
REM Use in drawing
GCOL RGB(0, 255, 0)
RECTANGLE 10, 10, 100, 50
```

---

### Notes

* The return value is a 24-bit RGB colour packed into an integer.
* Valid range for each channel is 0–255; values outside this range are clipped.
* Typically used with graphics commands such as \ref GCOL "GCOL" and \ref RECTANGLE "RECTANGLE".

---

**See also:**
\ref GCOL "GCOL" · \ref RECTANGLE "RECTANGLE" · \ref POINT "POINT"
