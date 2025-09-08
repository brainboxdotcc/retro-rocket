\page DELETE DELETE Keyword
```basic
DELETE string-expression
```

Deletes the specified **file** from the filesystem.

- `string-expression` must evaluate to a **filename or path**.
- Paths are **case-insensitive**.
- Relative paths are resolved against the current directory (see \ref CHDIR "CHDIR").
- `.` and `..` are **not supported** in paths.


@note This is a **destructive** operation (no undo).
@note If the file does not exist, or cannot be removed (for example because it is currently **open**),
@note a runtime error is raised (catchable with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR)).


@note `DELETE` removes **files** only. To remove a directory, use \ref RMDIR "RMDIR".

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
- Ensure any open handles to the file are **closed** first with \ref CLOSE "CLOSE".
- Wildcards are not supported; delete files **one at a time**.
- Deleting a non-existent file raises an error.

**See also:**  
\ref OPENIN "OPENIN" ·
\ref OPENOUT "OPENOUT" ·
\ref OPENUP "OPENUP" ·
\ref CLOSE "CLOSE" ·
\ref RMDIR "RMDIR" ·
\ref CHDIR "CHDIR" ·
\ref FILETYPE "FILETYPE$"
