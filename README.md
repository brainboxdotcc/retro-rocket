# 🚀 Retro Rocket BASIC-Powered Operating System

![image](https://retrorocket.dev/leader3.png)

Retro Rocket is a small operating system in the spirit of the 1980s micros. Something you can actually understand, not just use.

It runs on real x86-64 hardware, has its own programming language for the shell and for writing programs inspired by BASIC, and doesn’t hide much. Everything is there for you to play with
if you want to dig into it, and you can just start writing programs immediately, right from the command prompt, without waiting for programs to compile. You can write games, internet utilities,
or anything you like.

You can sit down with it, try things, break things, and see what happens. The idea is that you’re always close to what the system is actually doing, not several layers removed from it.

Getting into low-level stuff can usually be a bit of a wall. You usually end up fighting tools, languages, and systems that assume you already know the answer. This is a more direct way in.

Retro Rocket meets you in the middle, letting you dive into the guts of your computer.

## 🌟 Features

* Command line shell powered by BASIC
* A bunch of useful command line tools, including:
  * A fully featured WYSIWYG editor
  * A simple IRC client
  * Telnet client
  * A web server!
* ️Graphics drawing commands
  * Image loading of JPG, PNG, GIF, BMP, TGA, PSD (and support for animated GIF with animation control)
  * Software rendered advanced sprite blitting
* 🔊Sound support for **AC'97**, **HD Audio** and **Xonar D1** cards with:
  * 64 channel polyphonic mixer
  * WAV support
  * MP3 support
  * FLAC support
  * OGG vorbis support
  * tracker MOD and XM support
  * ADSR envelopes and waveform synthesis
* Supports the following file systems:
  * RetroFS (*read/write*)
  * ADFS "L" format adl disk images (*read only*)
  * FAT32 (*read/write*)
  * ISO9660 (*read only*)
  * UDF (*read only*)
* Multitasking of BASIC programs
* Network stack with TCP, IP, UDP, and ICMP
  * TLS is supported
* Full installer to install the OS to:
  * NVMe
  * AHCI/SATA
  * VirtIO

## 💻 System Requirements

* Any `x86_64` system from 2012 onwards
* Or run under emulation via [QEMU](https://www.qemu.org/)
* Optionally: RTL8139, e1000, or RTL8169 network card - tested on real hardware!
* SATA AHCI compatible optical drive and hard drive/SSD, NVMe or VirtIO (as supported by QEMU)
* For installation and hard disk boot, UEFI is required
* UEFI required for USB boot, CSM required for CD-ROM boot

## 🔨 Documentation

You can learn all about the project via the [official website at http://retrorocket.dev](http://retrorocket.dev) where we document technical details, provide a user guide, and BASIC reference.

## Building

You should not need to build Retro Rocket yourself. Under the releases, and the actions for the project, you will find ready-built
ISO and USB images you can burn straight to media. If you do want to build it yourself, you will need the following build dependencies:

- gcc-14 or higher
- cmake
- make
- nasm
- xorriso
- mtools
- php 8.4 or higher

To build, make a build directory, e.g. `build`, and then run `cmake` referencing the source directory:

```
cd retrorocket
mkdir build
cd build
cmake ..
make -j ${NPROC}
```

the ISO and USB images will be written to the build directory.
