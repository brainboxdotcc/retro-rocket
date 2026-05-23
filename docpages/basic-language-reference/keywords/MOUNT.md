\page MOUNT MOUNT Keyword

```basic
MOUNT "mountpoint", "device", "fstype"
```

Mounts a filesystem of the specified type at the given **mount point**.

* **mountpoint**: path where the filesystem will appear in the directory tree (for example `"/harddisk"`).
* **device**: device identifier (for example `"hd0"` for the first hard disk). May be an **empty string** for virtual filesystems. A partition number may optionally be appended after a comma, for example `"hd0,0"` for the first partition on `hd0`. Partition numbers always start at 0, same as device numbers, and will work regardless of if the device has a GPT or BIOS partition table.
* **fstype**: one of the supported filesystem types listed below.

@note Supported filesystem types:
@note - rfs (Retro Rocket Filesystem)
@note - iso9660 (CD-ROM images and discs)
@note - fat32
@note - devfs
@note - DummyFS
@note - udf (DVD and Blu-ray media)
@note - dfs (Acorn DFS floppy images)
@note - adfs (Acorn ADFS volumes)
@note - ext2 (read-only)

Paths are **case-insensitive**. `.` and `..` are **not supported** in paths. If no partition number is specified, Retro Rocket scans the device for the first matching supported partition type and falls back to mounting the raw device if no partition table is found.

After mounting, files and directories at the mount point are accessible to normal file I/O (`OPENIN`, `OPENOUT`, `OPENUP`, `DELETE`, `MKDIR`, `RMDIR`, etc.). Use `CHDIR` to switch into the mounted path.

On any mount, successful or unsuccessful, a status message is output to the console. This generates no BASIC error on failure, to allow for graceful failure and continuation behaviour during boot.

---

### Examples

```basic
MOUNT "/devices", "", "devfs"
MOUNT "/harddisk", "hd0", "fat32"
MOUNT "/linux", "hd1,0", "ext2"
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
