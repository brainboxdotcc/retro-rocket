\page BACKGROUND BACKGROUND Keyword
```basic
BACKGROUND integer-expression
```

Sets the current **text background colour** to `integer-expression`.  
Colours use the standard **VGA 0–15 palette**, emitted via ANSI escape codes. The setting affects subsequent text output (e.g., `PRINT`) until changed or the screen is cleared.

![VGA background and foreground colour chart](https://user-images.githubusercontent.com/1556794/234695104-af3fd095-a0fe-4a69-85e9-9db698466caa.png)


@note Use \ref COLOUR "COLOUR" to set **foreground** and background together.
@note \ref GCOL "GCOL" affects **graphics drawing**, not text.

---

##### VGA Colour Indices

| Index | Name                 |
|------:|----------------------|
| 0     | Black                |
| 1     | Blue                 |
| 2     | Green                |
| 3     | Cyan                 |
| 4     | Red                  |
| 5     | Magenta              |
| 6     | Brown (Dark Yellow)  |
| 7     | Light Grey           |
| 8     | Dark Grey            |
| 9     | Bright Blue          |
| 10    | Bright Green         |
| 11    | Bright Cyan          |
| 12    | Bright Red           |
| 13    | Bright Magenta       |
| 14    | Bright Yellow        |
| 15    | White                |

---

##### Example

```basic
FOR C = 0 TO 15
    BACKGROUND C
    PRINT "BACKGROUND COLOUR "; C
NEXT
```

---

##### Notes
- Valid range is **0–15**. Values outside this range are **implementation-defined**.
- Background colour persists until changed, even after \ref CLS "CLS".
- This statement only affects **text-mode** rendering; use graphics statements for pixel fills.

**See also:**
\ref COLOUR "COLOUR", \ref GCOL "GCOL", \ref CLS "CLS"
