\page troubleshooting Troubleshooting

This page covers common issues you might encounter while using Retro Rocket, and how to fix them.

---

### System doesn’t boot from the CD
- Make sure your PC or virtual machine is set to boot from **CD/DVD** first or select it from the boot menu.
- Make sure your PC or virtual machine supports BIOS boot for **CD/DVD**. Our ISO image supports BIOS boot (for older machines).

---

### System doesn’t boot from the USB stick
- Make sure your PC or virtual machine is set to boot from **USB** first or select it from the boot menu.
- Make sure your PC or virtual machine supports UEFI boot for **USB**. Our USB stick image supports UEFI boot (for newer machines).

---

### Black screen or nothing happens after boot
- Check that your system supports **UEFI GOP** (or at least VESA graphics).
- If running in a virtual machine, try switching the graphics adapter (for example QEMU: `-vga std`).

---

### No network connection
- Retro Rocket only supports **one network card** at a time.
- By default, the **e1000 driver** is loaded, which works for most virtual machines and many real NICs.
- To try a different driver, edit `/programs/init` on an installed system.

---

### My work disappeared after reboot
- Files saved in `/ramdisk` are temporary and will vanish when you power off.
- To keep your work, either:
  - Install Retro Rocket to a hard disk, or
  - Copy your files over the network before shutting down.

---

### Programs won’t run
- Make sure you typed the program name correctly at the shell prompt.
- Programs live in `/programs`. Use `dir /programs` to see what is available.
- Remember: commands are usually lowercase, BASIC keywords are uppercase.

---

### I broke something in `/programs/init`
- If Retro Rocket fails to start properly because of changes you made, boot from the LiveCD or LiveUSB again.
- From there, you can reinstall or edit `/programs/init` on your installed system to fix it. (use @ref mount "mount" to mount your installed system to `/harddisk` and make changes using @ref edit "edit")

---

### Still stuck?
- If you’re running in a VM, try increasing the memory to at least **4 GB**.
- If you are using `qemu` use our recommended configuration from `run.sh`/`run.bat`
- Retro Rocket doesn’t aim to support very old hardware.
- You can also find help on [discord](https://discord.gg/SVRFhsptXn)
