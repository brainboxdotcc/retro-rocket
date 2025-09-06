\page RMDIR RMDIR Keyword
```basic
RMDIR string-expression
```

Removes the specified **directory** from the filesystem.

- `string-expression` must evaluate to a directory **name or path**.
- The directory **must be empty** or an error is raised.


\remark Paths are **case-insensitive**.
> `.` and `..` are **not supported** in paths.


> Use an **absolute** path when it starts with `/`; otherwise the path is **relative** to the current directory.  
> Change directory with [`CHDIR`](https://github.com/brainboxdotcc/retro-rocket/wiki/CHDIR).


> If removal fails, you can catch it with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR).

---

### Examples

Remove a directory in the current folder
```basic
RMDIR "old_data"
```

Remove a directory by absolute path
```basic
RMDIR "/projects/archive"
```

Create then remove (must be empty)
```basic
MKDIR "tmp"
RMDIR "tmp"
```

---

### Notes
- `RMDIR` removes **directories** only. To delete a **file**, use [`DELETE`](https://github.com/brainboxdotcc/retro-rocket/wiki/DELETE).
- Attempting to remove a non-empty directory raises a runtime error.
- Ensure you have permission and that no required files are inside before removing.

**See also:**  
[`MKDIR`](https://github.com/brainboxdotcc/retro-rocket/wiki/MKDIR) ·
[`DELETE`](https://github.com/brainboxdotcc/retro-rocket/wiki/DELETE) ·
[`CHDIR`](https://github.com/brainboxdotcc/retro-rocket/wiki/CHDIR)
