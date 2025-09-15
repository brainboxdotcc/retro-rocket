\page VDU VDU Keyword
[TOC]

```basic
VDU n [ , arg1 [ , arg2 ... ] ]
```

Issues a **screen control** command. `n` selects the command. Arguments (if any) follow as comma-separated integers. Unused trailing numbers are accepted and ignored.

Retro Rocket BASIC implements a practical subset of BBC MOS `VDU`. Unsupported commands are no-ops (their arguments are discarded).

---

### Notes

* Coordinates for cursor movement use **text cells**: `x` is the column (0..cols-1), `y` is the row (0..rows-1).
* `VDU 12` (**CLS**) and `VDU 16` (**CLG**) both clear the **entire** screen.
* `VDU 18` (**graphics colour**) accepts a full **32-bit 0xRetro RocketGGBB** value in Retro Rocket BASIC, rather than the BBC’s 3-bit/8-bit palette indices.
* `VDU 23` (**redefine character**) allows redefining codes **32..255** (8×8 bitmaps, one byte per row). Control codes < 32 are not redefinable.
* `VDU 27, n` writes the **single character** whose code is `n`.
* Extra numbers after a command are **ignored** (kept for compatibility with BBC syntax).

---

### Implemented commands

#### VDU 6
  Enable text output to the screen (no state change in Retro Rocket at present; treated as a no-op).

#### VDU 7
  Beep (fixed \~1 kHz chirp).

#### VDU 8
  Cursor left one character, wrapping to the previous line if `x = 0`.

#### VDU 9
  Cursor right one character, wrapping to the next line at end of row (scrolls if needed).

#### VDU 10
  Cursor down one line (scrolls at bottom).

#### VDU 11
  Cursor up one line (stops at top).

#### VDU 12
  Clear screen (CLS).

#### VDU 13
  Carriage return (move to column 0 of current line).

#### VDU 16
  Clear screen (CLG in BBC terms, but acts as CLS in Retro Rocket).

#### VDU 17, colour
  Set **text** colour to a VGA colour code between 0 and 15.

#### VDU 18, colour
  Set **graphics** colour to a **32-bit** RGB value (e.g. `&FF00FF`).
  (BBC used palette indices; Retro Rocket uses true-colour.)

#### VDU 20
  Reset graphics colour to default (**white**, `&FFFFFF`), and text colour to default (**white**, 0xA0A0A0)

#### VDU 23, code

  ```
    VDU 23, code, b0, b1, b2, b3, b4, b5, b6, b7
  ```
  Redefine glyph for character `code` (32–255). Each `bN` is an 8-bit row of the 8×8 bitmap; the **most significant bit** (bit 7) is the **leftmost pixel**, and the **least significant bit** (bit 0) is the **rightmost pixel**. A `1` draws the foreground colour, a `0` leaves the background.

  * **UTF-8 interaction:** If a character has been redefined, it is always treated as a literal 8-bit glyph, never as part of a UTF-8 sequence. This ensures reliable rendering in both `PRINT` and `GRAPHPRINT`.
  * **Dynamic effect:** Once a glyph is redefined, all **existing screen contents** of that code can update to use the new shape. For example if screen scrolls, the new form is used.
  * **Limits:** Only codes ≥ 32 may be redefined. Control codes (< 32) are reserved.
  * **Persistence:** Redefinitions remain until changed again or reset.

  **Example: alien invader**

```basic
  VDU 23,65,&3C,&7E,&FF,&E7,&FF,&66,&66,&00
  PRINT "BEEP BOOP BEEP BOOP: A"
```

  Produces a bitmap like:

\image html vdu23.jpg

  **Example: simple animation**

```basic
  VDU 23,66,&18,&3C,&7E,&18,&18,&18,&18,&00  ' Frame 1
  PRINT "B";
  SLEEP 1
  VDU 8
  VDU 23,66,&00,&18,&3C,&7E,&18,&18,&18,&00  ' Frame 2
  PRINT "B"
```

  * Special sub-codes (< 32):

    * `VDU 23,1,enable` — **show/hide cursor** (0 = hide, non-zero = show).

#### VDU 25, …
  **Plot line** In Retro Rocket this delegates to the `LINE` statement; supply parameters exactly as for `LINE x1, y1, x2, y2`.

#### VDU 26
  Reset text and graphics windows to defaults (full screen) as defined by `VDU 28`

#### VDU 27, n
  Output the single character with code `n`.

#### VDU 28, …
  Define a **text window**. Only the top and bottom of the window are valid in Retro Rocket, the x coordinates are ignored.

#### VDU 30
  Home cursor to (0,0).

#### VDU 31, x, y
  Move cursor to text position `(x, y)`.

#### VDU 127
  Destructive backspace (mapped to BS).

---

### Differences vs BBC Micro

* **Only one VDU command per line** - You may not stack multiple VDU commands in one long sequence, this is to aid readability and prevent spamming of the console subsystem to aid multitasking fairness.
* **Printer channel control:** `VDU 0..3` - not supported (ignored).
* **Text and graphics print routing:** `VDU 4` (text), `VDU 5` (graphics) - **not applicable** in Retro Rocket. Retro Rocket uses **immediate-mode graphics**; use `GRAPHPRINT x, y, ...` for pixel text instead of `VDU 5`.
* **Auto-paging:** `VDU 14`, `VDU 15` - not supported.
* **Palette programming:** `VDU 19` - not supported (Retro Rocket is true-colour; use `VDU 18` / `GCOL`).
* **VDU off until 6:** `VDU 21` - not supported.
* **Mode change:** `VDU 22` - not supported (Retro Rocket has fixed text geometry; use graphics APIs for rendering).
* **Set graphics window:** `VDU 24` not supported (Retro Rocket does not implement VDU windows).
* **Set graphics origin:** `VDU 29` - not supported (Retro Rocket graphics are immediate-mode with explicit coordinates).
* **VDU 23 sub-codes:** only `23,1,enable` (cursor show/hide) is implemented; others (cursor width, scroll viewport, cursor motion defaults) are ignored.
* **VDU 16 (CLG)** clears the **whole** screen in Retro Rocket (on BBC it cleared only the graphics viewport).
* **Colour depth:** Retro Rocket accepts **32-bit** RGB for `VDU 18`; BBC used limited palette indices.

---

### Examples

**Redefine te bitmap for “Z” and print it**

```basic
VDU 23,ASC('Z'),&3C,&7E,&FF,&E7,&FF,&66,&66,&00
PRINT "ALIEN: Z"
```

**Set graphics colour to magenta and draw a line (VDU 25 delegates to LINE)**

```basic
VDU 18, &FF00FF
VDU 25, 100, 120, 220, 160
```

**Move the text cursor and write a single character**

```basic
VDU 31, 10, 5
VDU 27, 42     ' prints "*"
```

**Clear screen and home**

```basic
VDU 12
VDU 30
```

---

### See also

* \ref GRAPHPRINT "GRAPHPRINT" - draw text at pixel coordinates (immediate-mode overlay).
* \ref GCOL "GCOL" - set graphics colour (0xRetro RocketGGBB).
* \ref LINE "LINE" - draw lines (used by `VDU 25` in Retro Rocket).
* \ref PRINT "PRINT" - standard text output to the scrolling text grid.
