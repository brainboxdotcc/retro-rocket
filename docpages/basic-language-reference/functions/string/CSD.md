\page CSD CSD$ Function

```basic
CSD$
```

Returns the **current working directory** for the process as a string.
The result is always an **absolute path** from the virtual file system root (`/`).

Newly spawned processes inherit the current directory of their parent process.

---

### Examples

```basic
PRINT CSD$
```

Might produce:

```
/home/user/projects
```

```basic
REM Save current directory, change, then restore
oldDir$ = CSD$
CHDIR "/tmp"
PRINT "Now in "; CSD$
CHDIR oldDir$
PRINT "Back in "; CSD$
```

---

### Notes

* The returned path always begins with `/`.
* A process’ working directory may be changed with \ref CHDIR "CHDIR".
* Child processes inherit the parent’s working directory at the moment they are spawned.

---

**See also:**
\ref CHDIR "CHDIR" · \ref GETNAMECOUNT "GETNAMECOUNT" · \ref GETSIZE "GETSIZE"
