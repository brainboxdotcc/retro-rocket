\page MOUNT MOUNT Keyword
```basic
MOUNT "mountpoint", "device", "fstype"
```

Mounts a filesystem of the specified type at the given **mount point**.

- **mountpoint**: path where the filesystem will appear in the directory tree (for example `"/harddisk"`).
- **device**: device identifier (for example `"hd0"` for the first hard disk). May be an **empty string** for virtual filesystems.
- **fstype**: one of the supported filesystem types listed below.


\remark Supported filesystem types:
\remark - `iso9660`
\remark - `fat32`
@note - `devfs`
@note - `DummyFS`
@note - `rfs`


@note Paths are **case-insensitive**. `.` and `..` are **not supported** in paths.


@note After mounting, files and directories at the mount point are accessible to normal file I/O
@note (`OPENIN`, `OPENOUT`, `OPENUP`, `DELETE`, `MKDIR`, `RMDIR`, etc.). Use `CHDIR` to switch into the mounted path.

If the mount fails (for example, unsupported `fstype` or unavailable device), a runtime error is raised (catchable with
[`ON ERROR`](https://github.com/brainboxdotcc/retro-rocket/wiki/ONERROR)).

---

### Examples
```basic
MOUNT "/devices", "", "devfs"
MOUNT "/harddisk", "hd0", "fat32"
```

---

**See also:**  
\ref CHDIR "CHDIR" ·
\ref MKDIR "MKDIR" ·
\ref DELETE "DELETE" ·
\ref RMDIR "RMDIR" ·
\ref OPENIN "OPENIN" ·
\ref OPENOUT "OPENOUT" ·
\ref OPENUP "OPENUP"
