\page KEYMAP KEYMAP Keyword
```basic
KEYMAP string-expression
```

Loads a **keyboard mapping** by name from the `/system/keymaps` directory.

- The value of `string-expression` should be the **map name** (for example `"en-GB"`).  
- The interpreter will try to load `/system/keymaps/<name>.keymap`.  
  Example: `KEYMAP "en-GB"` → `/system/keymaps/en-GB.keymap`.
- Paths are **case-insensitive**.  
- `.` and `..` are **not supported** in paths.


@note Only the keymaps that actually exist under `/system/keymaps` can be loaded.
@note Do not assume a particular layout is available.


@note If a keymap cannot be found or loaded, a runtime error is raised (catchable with
@note [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR)).

---

##### Example

```basic
REM Load British English keyboard layout (if installed)
KEYMAP "en-GB"
```

---

##### Notes
- The setting affects how keyboard input is interpreted by the system.  
- To see which keymaps are present, inspect the `/system/keymaps` directory with your usual tools.
