\page keymaps Keyboard Layouts

Your keyboard layout is defined by a configuration file called a keymap, which controls what characters appear when you press keys.

They are used to change which key is mapped to which character add characters that are not on your keyboard, or not defined by the
base set of characters.

Keymaps are stored in:

```
/system/keymaps/
```

For example:

```
/system/keymaps/en-GB.keymap
```

A keymap can be loaded at runtime by the \ref KEYMAP.

---

### Format

Each line is one of:

* a key mapping
* a symbol definition
* a comment (`# ...`)

---

### Key mappings

```
&SCANCODE normal shifted
```

This defines what a key produces:

* `normal` is the character without Shift
* `shifted` is the character with Shift

Example:

```
&02 1 !
&04 3 &A3
```

This makes the `3` key produce `£` when Shift is held.

---

### Symbol definitions

```
SYM &XX &b0 &b1 &b2 &b3 &b4 &b5 &b6 &b7
```

Defines how a character looks on screen.

* `&XX` is the character number (the `&` means the number is written in hex)
* the eight values that follow are the 8 rows of the character
* each value is a number from 0 to 255
* each bit in that number controls one pixel (1 = on, 0 = off)
* the character is 8 pixels high and 8 pixels wide

Example (`£`):

```
SYM &A3 &1C &36 &30 &7C &30 &30 &7E &00
```

This defines the shape used whenever character `&A3` is printed.

---

### How to read a row

Take this value:

```
&1C
```

In binary, this is:

```
00011100
```

This means:

```
...###..
```

Each `#` is a lit pixel, each `.` is empty.

---

### How it is used

If a key is mapped to a character number, and that character has a symbol definition, that symbol will be shown when the key is pressed.

Example:

```
&04 3 &A3
SYM &A3 &1C &36 &30 &7C &30 &30 &7E &00
```

Pressing `Shift+3` will produce `£`.

---

### Notes

* symbol definitions only affect how a character looks
* redefining a character replaces any previous definition
* characters without a definition use the default font
* keymaps override the built-in layout
* only keys that are listed are changed
* duplicate entries are ignored
