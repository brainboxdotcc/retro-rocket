\page LIBRARY LIBRARY Keyword
```basic
LIBRARY string-expression
```

Loads a **library** (a file of `PROC`/`FN` definitions with **no line numbers**) into the current program.  
Immediately after `LIBRARY`, any procedures or functions from that file are available to call.

- `string-expression` is the **path or name** of the library file.
- Relative paths are resolved from the current directory; absolute paths start with `/`.


> A library file **must not contain line numbers**.  
> If a line-numbered program loads a library, the library is **auto-numbered** on load so its lines come **after** the program’s existing lines (using increments of 10). See [Automatic line numbering](https://github.com/brainboxdotcc/retro-rocket/wiki/Automatic-line-numbering).


> On load, Retro Rocket looks for a procedure with the **same name as the library filename** and **calls it** if found.  
> This acts as the library’s **constructor** for one-time initialisation.


> It is recommended to place `LIBRARY` statements **near the start** of your program.  
> Loading a library has a non-trivial cost.


> Use the standard global `LIB$` path when loading shared libraries:
> ```basic
> LIBRARY LIB$ + "/ansi"
> ```
> `LIB$` is set by `init` and points to an absolute library folder.


> Paths are **case-insensitive** and `.` / `..` are **not supported** in paths.

---

### Examples

**Load a library from `LIB$` and call its code**
```basic
LIBRARY LIB$ + "/ansi"
PROCansi_demo
```

**Load a local library and use a function**
```basic
LIBRARY "mathlib"
PRINT FNadd(2, 3)
```

**Library file (no line numbers)**
```basic
REM File: mathlib

DEF FNadd(A, B)
= A + B

DEF PROCmathlib
    REM optional constructor work here
ENDPROC
```

---

### Errors and behaviour
- If the library file **cannot be found or loaded**, a runtime error is raised (catch with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR) if needed).
- After loading, all `PROC`/`FN` names from the library become available in the current program.
- The constructor call (procedure named after the file) is **optional**. If absent, nothing extra is called.

**See also:**  
[`DEF`](https://github.com/brainboxdotcc/retro-rocket/wiki/DEF) ·
[`PROC`](https://github.com/brainboxdotcc/retro-rocket/wiki/PROC) ·
[`FN`](https://github.com/brainboxdotcc/retro-rocket/wiki/FN) ·
[`Automatic line numbering`](https://github.com/brainboxdotcc/retro-rocket/wiki/Automatic-line-numbering) ·
[`ansi`](https://github.com/brainboxdotcc/retro-rocket/wiki/ansi)
