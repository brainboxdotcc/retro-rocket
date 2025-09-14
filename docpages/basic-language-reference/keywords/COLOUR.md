\page COLOUR COLOUR / COLOR Keyword
```basic
COLOUR integer-expression
```

Sets the **text foreground colour** to a VGA palette entry (0–15).  
The default terminal colour is **7** (white).

\image html colours.png


@note Both British and American spellings are accepted: `COLOUR` and `COLOR`.

---

##### Example

```basic
COLOUR 14
PRINT "This text is bright yellow."
```

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

##### Notes
- Applies to **text output** (e.g., `PRINT`).  
- The setting affects subsequent text until changed again.  
- For **background** colour, use \ref BACKGROUND "BACKGROUND".
- For **graphics drawing colours**, use \ref GCOL "GCOL".

**See also:**
\ref BACKGROUND "BACKGROUND", \ref GCOL "GCOL"
