\page mount-command mount command

## mount [directory] [storage-device-name] [filesystem-type]

Attaches a file system on a storage device to the visible set of directories and files you can see and work with.

Think of mounting a filesystem like plugging in something to a power outlet. Once you have plugged it in and turned it on, it is available to use. This is basically how file systems work.

- The first parameter is the directory, this can be any existing empty directory in the file system. For example /harddisk, /ramdisk, etc.
- The second parameter is a storage device name. Storage device names are allocated by the system. Hard disks (SATA, NVMe etc) start with `hd` and are suffixed by number starting at 0, so the first found hard disk is `hd0`, the next `hd1` etc. Ram disks start with `ram` amd a number. CD/DVD drives start with `cd` and a number.
- The third parameter is a filesystem type, supported file system types are listed below.

| File system type | Full name                                    | Description                                                                                                                      |
|------------------|----------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| rfs              | RetroFS, the Retro Rocket native file system | Readable, writeable file system for large hard disks, e.g. NVMe, SATA. The default file system type for fixed disks and ramdisks |
| iso9660          | ISO 9660 file system for CDs and DVDs        | Read-only file system mainly for CDs and DVDs. The default file system of the Retro Rocket LiveCD                                |
| fat32            | FAT32 DOS and Windows file system            | Readable, writeable file system used for removable media, and the recovery/boot partition of installed systems                   |
| devfs            | Device tree file system                      | Readable, writeable file system which device drivers may use to expose device-specific information and control surfaces. *       |
| dummyfs          | Dummy File System                            | The dummy file system does nothing, you cannot read or write to it. It is the placeholder for everything until the system boots  |

\* For `devfs`, you cannot create arbitrary files in this file system. Your ability to read from, or write to the files in this directory depends on the behaviour of the driver which places the file there.

### Example

```
mount /harddisk hd0 rfs
```

### Notes
- Mounting a storage device with a file system it is not configured for will result in an error
- Unmounting file systems is **not yet supported**. To clear a mounting, it is currently required that you reboot the system.