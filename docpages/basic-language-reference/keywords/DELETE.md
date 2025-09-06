\page DELETE DELETE Keyword
```basic
DELETE string-expression
```

Deletes the specified **file** from the filesystem.

- `string-expression` must evaluate to a **filename or path**.
- Paths are **case-insensitive**.
- Relative paths are resolved against the current directory (see [`CHDIR`](https://github.com/brainboxdotcc/retro-rocket/wiki/CHDIR)).
- `.` and `..` are **not supported** in paths.


> This is a **destructive** operation (no undo).  
> If the file does not exist, or cannot be removed (for example because it is currently **open**),
> a runtime error is raised (catchable with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR)).


> `DELETE` removes **files** only. To remove a directory, use [`RMDIR`](https://github.com/brainboxdotcc/retro-rocket/wiki/RMDIR).

---

##### Example: simple delete

```basic
DELETE "old_config.txt"
```

---

##### Example: defensive delete (only if it’s a file)

```basic
IF FILETYPE$("backup.dat") = "FILE" THEN
    DELETE "backup.dat"
ELSE
    PRINT "Not a file, skipping."
ENDIF
```

---

##### Notes
- Ensure any open handles to the file are **closed** first with [`CLOSE`](https://github.com/brainboxdotcc/retro-rocket/wiki/CLOSE).
- Wildcards are not supported; delete files **one at a time**.
- Deleting a non-existent file raises an error.

**See also:**  
[`OPENIN`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENIN) ·
[`OPENOUT`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENOUT) ·
[`OPENUP`](https://github.com/brainboxdotcc/retro-rocket/wiki/OPENUP) ·
[`CLOSE`](https://github.com/brainboxdotcc/retro-rocket/wiki/CLOSE) ·
[`RMDIR`](https://github.com/brainboxdotcc/retro-rocket/wiki/RMDIR) ·
[`CHDIR`](https://github.com/brainboxdotcc/retro-rocket/wiki/CHDIR) ·
[`FILETYPE$`](https://github.com/brainboxdotcc/retro-rocket/wiki/FILETYPE)
