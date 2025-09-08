\page CHDIR CHDIR Keyword
```basic
CHDIR string-expression
```

Changes the **current working directory** (CSD) of the running process to the path given by `string-expression`.

- A path starting with `/` is treated as an **absolute** path.  
- A path not starting with `/` is treated as **relative** to the current CSD.  
- The initial process (`/programs/init`) starts with its CSD set to `/` (the root of the virtual filesystem).  
- Any child process launched (via \ref CHAIN "CHAIN", for example) inherits the parent’s current directory at the time of launch.


\remark Unlike Unix, Retro Rocket BASIC paths do **not** support `.` or `..`.
\remark Directory traversal is only possible by specifying absolute or relative names explicitly.

---

#### Example

```basic
CHDIR "/programs"
CHAIN "demo"
```

This changes the current directory to `/programs`, then launches `demo` from that directory.

---

##### Notes
- Paths must exist; if the target directory is invalid, an error occurs (catchable with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR)).  
- Paths are **case-insensitive**.  
- `rocketsh` provides a built-in command `chdir` (or short form `cd`) which performs the same action directly from the shell.  
- Changing the current directory affects file lookups for `OPENIN`, `CHAIN`, `LIBRARY`, and similar statements.

**See also:**
\ref CHAIN "CHAIN", \ref LIBRARY "LIBRARY"
