\page REM REM Keyword
```basic
REM any-text
```

Adds a remark (comment) to your program. The interpreter **ignores the rest of the line** after `REM`.


\remark
\remark You can use the ' symbol in any line as a shorthand to place a comment on the end of that line. e.g.
\remark
\remark ```basic
\remark PRINT "owls" ' Show my favourite bird
\remark ```
\remark You do not need to use any special separator, the ' on its own is enough to indicate where the comment begins.
\remark Inside a **string literal** (delimited by double quotes), an apostrophe is just a character:
\remark it does **not** start a comment.
\remark Example:
\remark ```basic
> PRINT "It's fine"` ' Prints the apostrophe
> ```

---

### Behaviour
- Everything from `REM` to the end of the line is treated as a comment.
- `REM` has **no runtime effect** and produces no output.
- You can place `REM` anywhere a statement is allowed (for example between executable lines).

---

### Examples

**Simple comment**
```basic
REM This program draws a box
GCOL RGB(0,255,0)
RECTANGLE 50, 50, 200, 120
```

**Temporarily disable a line**
```basic
REM PRINT "Debug: entering main loop"
```

**Empty remark (acts like a labelled blank line)**
```basic
REM
PRINT "Hello"
```

---

### Tips
- Use consistent prefixes in your comments (e.g. `REM TODO:` or `REM NOTE:`) to make them easy to search for.
- Keep comments adjacent to the code they describe so they stay in sync as the program evolves.
