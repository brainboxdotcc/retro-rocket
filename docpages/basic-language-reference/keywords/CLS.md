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
- `CLS` removes previously drawn graphics (e.g. from [`LINE`](https://github.com/brainboxdotcc/retro-rocket/wiki/LINE), [`RECTANGLE`](https://github.com/brainboxdotcc/retro-rocket/wiki/RECTANGLE), [`CIRCLE`](https://github.com/brainboxdotcc/retro-rocket/wiki/CIRCLE), etc.) as well as text.
- Colours set with [`COLOUR`](https://github.com/brainboxdotcc/retro-rocket/wiki/COLOUR) or [`BACKGROUND`](https://github.com/brainboxdotcc/retro-rocket/wiki/BACKGROUND) **remain in effect** after a clear.

**See also:** [`COLOUR`](https://github.com/brainboxdotcc/retro-rocket/wiki/COLOUR), [`BACKGROUND`](https://github.com/brainboxdotcc/retro-rocket/wiki/BACKGROUND)
