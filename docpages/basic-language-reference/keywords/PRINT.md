\page PRINT PRINT Keyword
```basic
PRINT printable
```

Outputs text at the current cursor position on the text screen.  
`printable` may be a string or a numeric expression; numbers are converted to text automatically.

---

### Separators and line endings

`PRINT` accepts multiple items separated by `;` or `,`:

- `;` prints the next item immediately with no extra spacing.
- `,` advances to the next print zone (tab stop) before printing the next item.
- If the line ends with `;` or `,`, no newline is emitted.
- With no arguments, `PRINT` outputs just a newline.


\remark Keep the cursor on the same line for a prompt:
\remark ```basic
@note PRINT "Enter name: ";
@note ```

---

### Using ANSI escape sequences (advanced)

`PRINT` can emit ANSI/VT100 escape sequences directly.  
The escape character is `ESC`, which is `CHR$(27)`. Most sequences begin with `ESC` + `"["` (Control Sequence Introducer).


@note `COLOUR` and `BACKGROUND` change text colours in a portable, high-level way.
@note ANSI escapes give lower-level control (cursor movement, attributes, manual colours, clearing parts of the screen, etc.).

Define a convenience prefix:
```basic
ESC$ = CHR$(27) + "["
```

**Text attributes and colours (SGR)**
```basic
REM Set red foreground, then reset
PRINT ESC$ + "31m" + "Red text" + ESC$ + "0m"

REM Bright yellow on blue background
PRINT ESC$ + "1m" + ESC$ + "33m" + ESC$ + "44m" + "Title" + ESC$ + "0m"
```
Common SGR parameters:
- `0` reset all attributes
- `1` bold/bright
- `30`..`37` foreground colours (black, red, green, yellow, blue, magenta, cyan, white)
- `40`..`47` background colours
- `90`..`97` bright foreground
- `100`..`107` bright background

**Cursor positioning and movement**
```basic
REM Home to 1,1
PRINT ESC$ + "H"

REM Move to row 10, column 20 (1-based)
PRINT ESC$ + "10;20H"

REM Relative moves: up/down/forward/back
PRINT ESC$ + "2A"   REM up 2
PRINT ESC$ + "5C"   REM right 5
```

**Erasing**
```basic
REM Clear entire screen and home
PRINT ESC$ + "2J" + ESC$ + "H"

REM Clear to end of line
PRINT ESC$ + "K"
```

**Show/hide cursor**
```basic
PRINT ESC$ + "?25l"   REM hide
PRINT ESC$ + "?25h"   REM show
```


@note Always finish coloured or styled output with `ESC[0m` to reset attributes, or later text may inherit your styles.


@note For higher-level helpers and key handling, consider the
@note [`ansi`](https://github.com/brainboxdotcc/retro-rocket/wiki/ansi) system library.

---

### Examples

Basic printing
```basic
PRINT "Hello"
PRINT 123
PRINT 1 + 2 * 3
```

Multiple items and spacing control
```basic
PRINT "A"; "B"; "C"
PRINT "Name", "Score", "Time"
```

Positioned output
```basic
CURSOR 10, 5
PRINT "Title"
```

Simple coloured prompt using ANSI
```basic
ESC$ = CHR$(27) + "["
PRINT ESC$ + "32m" + "OK> " + ESC$ + "0m";
```

---

### Notes
- Output affects the text layer. Use `COLOUR` and `BACKGROUND` for simple text colours, and `GCOL` for graphics drawing.
- The cursor advances as text is printed; move it explicitly with `CURSOR x,y` or ANSI positioning if needed.
- `CLS` clears both text and graphics. ANSI `ESC[2J` clears the text layer.

**See also:**  
\ref CURSOR "CURSOR" ·
\ref COLOUR "COLOUR" ·
\ref BACKGROUND "BACKGROUND" ·
[`ansi`](https://github.com/brainboxdotcc/retro-rocket/wiki/ansi)
