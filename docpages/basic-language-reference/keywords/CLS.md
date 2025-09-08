\page CLS CLS Keyword
```basic
CLS
```

Clears **everything on screen** - both the text display and any graphics - and moves the cursor to the **top-left** `(0,0)`.

- The current foreground and background **colours are not reset**.
- Subsequent text and graphics draw onto a blank screen using the active colours.

---

##### Example

```basic
PRINT "Hello!"
CIRCLE 200, 150, 50, TRUE
SLEEP 1
CLS
PRINT "Screen cleared."
```

---

##### Notes
- `CLS` removes previously drawn graphics (e.g. from \ref LINE "LINE", \ref RECTANGLE "RECTANGLE", \ref CIRCLE "CIRCLE", etc.) as well as text.
- Colours set with \ref COLOUR "COLOUR" or \ref BACKGROUND "BACKGROUND" **remain in effect** after a clear.

**See also:**
\ref COLOUR "COLOUR", \ref BACKGROUND "BACKGROUND"
