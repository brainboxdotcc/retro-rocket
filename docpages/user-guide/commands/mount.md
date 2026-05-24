\page mount-command mount command

```
mount <directory> <storage-device-name> <file-system-type>
```

Attaches a file system on a storage device to the visible set of directories and files you can see and work with.

Think of mounting a file system like plugging in something to a power outlet. Once you have plugged it in and turned it on, it is available to use. This is basically how file systems work.

* The first parameter is the directory, this can be any existing empty directory in the file system. For example /harddisk, /ramdisk, etc.
* The second parameter is a storage device name. Storage device names are allocated by the system. Hard disks (SATA, NVMe etc) start with `hd` and are suffixed by number starting at 0, so the first found hard disk is `hd0`, the next `hd1` etc. Ram disks start with `ram` and a number. CD/DVD drives start with `cd` and a number. A partition number may optionally be appended after a comma, for example `hd0,0` for the first partition on the first hard disk. Partition numbers always start at 0 and work regardless of whether the disk uses a GPT or BIOS partition table. You can list devices using the \ref devices-command
* The third parameter is a file system type, supported file system types are listed below.

| File system type | Full name                                    | Partition support | LVM2 support | Description                                                                                                                      |
| ---------------- | -------------------------------------------- | ----------------- | ------------ | -------------------------------------------------------------------------------------------------------------------------------- |
| rfs              | RetroFS, the Retro Rocket native file system | Yes               | No           | Readable, writeable file system for large hard disks, e.g. NVMe, SATA. The default file system type for fixed disks and ramdisks |
| iso9660          | ISO 9660 file system for CDs and DVDs        | No                | No           | Read-only file system mainly for CDs and DVDs. The default file system of the Retro Rocket LiveCD                                |
| adfs             | Acorn ADFS "L" Format                        | No                | No           | Read-only file system for Acorn BBC Micro Floppy disks - The Advanced Disk Filing System                                         |
| dfs              | Acorn DFS DS/DD 80T Format                   | No                | No           | Read-only file system for Acorn BBC Micro Floppy disks - The Disk Filing System                                                  |
| fat32            | FAT32 DOS and Windows file system            | Yes               | No           | Readable, writeable file system used for removable media, and the recovery/boot partition of installed systems                   |
| ext2             | Linux Extended File system (ext2, ext3)      | Yes               | Yes          | Read-only file system used on Linux systems. Supports direct mounting from supported simple linear LVM2 logical volumes          |
| udf              | UDF (Universal Disk Format)                  | No                | No           | Read-only file system used for removable media such as CD/DVD/Blu-Ray. Often seen on rewritable or multi-session media.          |
| devfs            | Device tree file system                      | No                | No           | Readable, writeable file system which device drivers may use to expose device-specific information and control surfaces. *       |
| dummyfs          | Dummy File System                            | No                | No           | The dummy file system does nothing, you cannot read or write to it. It is the placeholder for everything until the system boots  |

### Devfs Behaviour

For `devfs`, you cannot create arbitrary files in this file system. Your ability to read from, or write to, files in this directory depends on the behaviour of the driver which places them there.

### LVM2 Support

Simple linear LVM2 logical volumes are folded into the same visible partition numbering sequence as ordinary partitions. For example, if a disk contains four ordinary partitions followed by two supported LVM2 logical volumes, the logical volumes appear as partition numbers 4 and 5.

Retro Rocket currently supports read-only mounting of simple linear LVM2 logical volumes only. Striped, mirrored, RAID, snapshot, thin-provisioned, cached and multi-segment logical volumes are not currently supported.

### Whole-volume filesystems

For file system types which do not support partitions, any supplied partition number is silently ignored and the entire device is treated as the volume to be mounted.

## Example

```
mount /harddisk hd0 rfs
mount /linux hd1,0 ext2
mount /linux-lvm hd0,5 ext2
```

## Notes

* Attempting to mount a storage device using the wrong file system type will result in a status message
* If no partition number is specified, the system scans for the first compatible partition or supported logical volume and falls back to mounting the raw device if no partition table is found
* Unmounting file systems is **not yet supported**. To clear a mounting, it is currently required that you reboot the system.
