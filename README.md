# 🚀 Retro Rocket BASIC-Powered Operating System

![image](https://retrorocket.dev/leader3.png)

Imagine an alterate present time, where the [BBC Micro](https://en.wikipedia.org/wiki/BBC_Micro) had continued to thrive and develop to the present day. In this alternate present this operating system exists alongside Windows and Linux but stands apart.

This is an alternative operating system inspired by [Acorn MOS 3.20](https://en.wikipedia.org/wiki/Acorn_MOS), but modernised for current hardware as a thought experiment and a bit of a toy to have fun with.

Its userland is completely written in a dialect of BASIC based heavily on BBC BASIC, with full access to hardware (just like the old days), SMP, multi-tasking, modern filesystem support, internet utilities and more as well as planned backwards compatibility with Acorn features such as ADFS via disk images.

## 🌟 Features

* 💻Command line shell powered by BASIC
* 🧰A bunch of useful command line tools, including:
  * 📜A fully featured WYSIWYG editor
  * 💬A simple IRC client
  * 🌐A web server!
* ✏️Graphics drawing commands
  * Image loading of PNG, GIF, BMP, TGA
* 🔊Sound support for AC'97 and HD Audio cards with:
  * 64 channel polyphonic mixer
  * wav support
  * mp3 support
  * flac support
  * ogg vorbis support
  * tracker mod and xm support
  * ADSR envelopes and waveform synthesis
* 💽Virtual File System with support for:
  * RetroFS (*read/write*)
  * FAT32 (*read/write*)
  * ISO9660 (*read only*)
* ✅Multitasking of BASIC programs
* 📶Network stack with TCP, IP, UDP, and ICMP
  * TLS is supported
* 📦Full installer to install the OS to:
  * NVMe
  * AHCI/SATA
  * VirtIO

## 💻 System Requirements

* An `x86_64` system with CD or DVD drive to boot an ISO image, or installation from hard disk
* Or run under emulation via [QEMU](https://www.qemu.org/)
* Optionally: RTL8139 or e1000 network card - tested on real hardware!
* SATA AHCI compatible optical drive and hard drive/SSD (as supported by QEMU) - IDE is no longer supported!

## 🔨 Technical Details

This operating system boots via the [limine bootloader](https://github.com/limine-bootloader/limine) and runs only on 64 bit systems. Where possible it will take advantage of SMP systems (multi-core) - this part of the OS is in the process of being rewritten.

When booted, the OS will first run `/programs/init` which is a simple BASIC script which will `CHAIN` the shell, `rocketsh`. `rocketsh` supports direct BASIC commands (e.g. PRINT, variable assignment, function and procedure calls, EVAL, etc). Any unknown instructions will be interpreted as a command, and `rocketsh` will search for a matching program name in `/programs`
which will be `CHAIN`ed.

Note that in this operating system the `CHAIN` instruction does **not replace** the current program with a new one (on the BBC Micro it did this, much like POSIX `exec()`) instead, it places the current program into a waiting state, starts the new program, and at the point the new program ends, continues the old program from where it left off. Using this you can spawn complex trees of related program functionality. The ability to `CHAIN` programs to the background and immediately return is coming soon™.
