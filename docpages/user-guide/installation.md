\page installation Installation

Retro Rocket can be installed from the LiveCD onto a SATA or NVMe hard disk.
Installation is simple and text-based.

@warning Installing Retro Rocket will erase all data on the target drive. Dual-boot is not supported!

---

### 1. Boot the LiveCD
Insert the Retro Rocket CD and boot from it.
On the boot menu, choose **Install Retro Rocket** with the arrow keys and press **Enter**.

\image html install-menu.png

---

### 2. Select a drive
The installer shows a list of available drives. Each drive is listed with its size and type.
Press the number key for the drive you want to install to.

\image html install-drive.png

@warning All existing data on the chosen drive will be lost.

---

### 3. Confirm your choice
You will be asked to confirm.
Press **Y** to continue or **N** to go back and choose another drive.

\image html install-confirm.png

---

### 4. Copying files
The installer will copy the system files from the CD to your hard disk.
A progress bar shows the copy process.

\image html install-copy.png

---

### 5. Installation complete
When finished, the installer will show a success message.
Remove the CD and press any key to reboot into your new Retro Rocket installation.

\image html install-done.png

---

### Notes

- Only SATA and NVMe drives supporting UEFI boot (modern motherboards from 2012 onwards) are supported.
- IDE/PATA and BIOS boot is not supported.
- The installer erases the entire target disk. There is no dual-boot support.
- The installed system uses the same directory layout as the LiveCD, but `/harddisk` is no longer used.
- All directories in the file system will be writeable to you, as Retro Rocket is a single-user system.