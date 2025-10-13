\page HIGHLIGHT HIGHLIGHT$ Function

```basic
HIGHLIGHT$(string-expression)
```

Given a string containing BASIC source, returns a copy with ANSI SGR escape sequences applied for syntax highlighting. Intended for program listings, editors, and tools that render to an ANSI-capable terminal, producing a consistent look-and-feel.

**Notes**

* **Strings** - text between double quotes (`"..."`) is highlighted in **light yellow** from the opening quote to the closing quote. If a closing quote is missing, highlighting continues to the end of the input and is then reset.
* **Comments**

  * A single quote (') begins a comment that is highlighted in **dark green** until the end of the input.
  * The keyword `REM` (when not inside a string) causes the **entire line** to be treated as a comment and highlighted in **dark green**.
* **Keywords / tokens** - recognised BASIC keywords are highlighted in **light blue** when they are not part of a longer identifier. Boundary detection prevents colouring inside names like `FORTH`. Exceptions: `PROC`, `FN`, and `=` may be adjacent to letters/digits/underscores and will still be highlighted.
* **Built-in functions**

  * Integer-returning built-ins are **light green**.
  * String-returning built-ins are **light magenta**.
  * Real/float-returning built-ins are **light cyan**.
* **Numeric literals** - digits (and a leading unary `+`/`-`) are highlighted in **orange**.
* **Operators & punctuation** - `() + - * / = < > , ;` are highlighted in **dark red**.
* **Default text** - rendered in **white**. Colours are reset back to white after each coloured span, and a final reset is appended at the end of the result to prevent “colour leakage” in the caller’s terminal.

**Errors**

* None

**Examples**

```basic
' Strings and a trailing comment
A$ = "PRINT " + CHR$(34) + "hello world" + CHR$(34) + " ' This is an example"
PRINT HIGHLIGHT$(A$)

' Keywords, numbers, and operators
B$ = "FOR I = 1 TO 10 + 8"
PRINT HIGHLIGHT$(B$)

' REM comment colours the whole line as a comment
D$ = "REM this whole line is a comment"
PRINT HIGHLIGHT$(D$)
```

**See also**

* \ref PRINT - display output on the screen
* \ref edit - Text editor
