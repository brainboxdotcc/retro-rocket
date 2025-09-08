\page MKDIR MKDIR Keyword
```basic
MKDIR string-expression
```

Creates a new **directory** in the filesystem.

- `string-expression` must evaluate to the directory **name or path**.
- Paths starting with `/` are **absolute**; other paths are **relative** to the current directory.


\remark Paths are **case-insensitive**, and `.` / `..` are **not supported** in paths.


\remark If the directory already exists, or the path refers to an existing **file**, an error is raised
\remark (catchable with [`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR)).

---

### Examples

Create a directory in the current folder
```basic
MKDIR "projects"
```

Create a directory by absolute path
```basic
MKDIR "/data/logs"
```

Change into the new directory
```basic
MKDIR "assets"
CHDIR "assets"
```

---

### See also
- \ref CHDIR "CHDIR" - change current directory
- \ref RMDIR "RMDIR" - remove a directory
- \ref DELETE "DELETE" - delete a file
