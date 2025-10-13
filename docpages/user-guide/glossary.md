\page glossary Glossary

This glossary explains common terms you will see in the Retro Rocket user guide.

---

### BASIC
A simple programming language that Retro Rocket uses for its programs.
BASIC stands for “Beginner’s All-purpose Symbolic Instruction Code.”

### Command
A word you type at the shell prompt (for example `dir`, `edit`, `up`).
Commands tell Retro Rocket to run a program or do something.

### Directory
A folder that contains files or other folders. Example: `/programs` is a directory.

### File system
The way files and directories are organised on a disk. Retro Rocket supports its own file system (RetroFS) as well as ISO 9660 and FAT32.

### LiveCD
Running Retro Rocket directly from the CD without installing it to a hard disk.
The system is read-only, but you get a temporary RAM disk to save your work.

### Mount
Attaching a file system to a directory so you can use it. Example: mounting a disk at `/ramdisk` makes it available there.

### Program
A file written in BASIC that Retro Rocket can run. Programs live in `/programs`.

### Prompt
The `>` symbol you see in the shell where you type commands.

### RAM disk
A temporary area of storage in memory, shown as `/ramdisk`. Fast, but everything is lost when you power off. Great for experiments and quick saves, but don’t trust it to keep your secrets after shutdown.

### Shell
The text-based interface where you type commands. In Retro Rocket this is called `rocketsh`.

### UEFI
Modern PCs use UEFI firmware instead of older BIOS. Retro Rocket boots on UEFI systems with standard graphics support.

### Driver

A small program that teaches Retro Rocket how to talk to a piece of hardware (like a network card or sound chip). Drivers are usually loaded during startup.

### DHCP

Short for *Dynamic Host Configuration Protocol*. A way for your computer to ask the network for its settings automatically (IP address, DNS, gateway). Default in Retro Rocket networking.

### DNS

Short for *Domain Name System*. Translates easy names (like `retro-rocket.net`) into the numbers (IP addresses) that computers use to talk to each other.

### Error

If something goes wrong, Retro Rocket will print an error message. Errors usually stop the current program, but you can always return to the shell prompt.

### Rickroll
When Retro Rocket cheekily reminds you that you’re never gonna give it up. Type `rick` at the shell and enjoy pixel art + chiptune pop.

### Pixel Art
Glorious blocky graphics that look like they were drawn with very large crayons. Retro Rocket uses these in demos and Easter eggs - not a bug, a feature.

### Loopback

The special network address `127.0.0.1`, which always points to your own machine. Useful for testing network programs locally.

### Module

Another word for a loadable program or driver. Modules are often used to add extra features without changing the whole system.

### Network interface

The connection point between Retro Rocket and the network (your network card or virtual adapter). Only one is supported at a time.

### Prompt string

The text shown before the `>` in the shell. It can be changed by editing the variable `PROMPT$`.

### RetroFS

The native Retro Rocket file system, designed for speed and simplicity on modern disks.

### Static IP

A fixed network address you type in manually, instead of getting one automatically via DHCP.

### Uptime

How long the system has been running since it was last started.

