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

* An `x86_64` system; CD/DVD or USB boot, or installation from hard disk
* Or run under emulation via [QEMU](https://www.qemu.org/)
* Optionally: RTL8139 or e1000 network card - tested on real hardware!
* SATA AHCI compatible optical drive and hard drive/SSD, NVMe or VirtIO (as supported by QEMU) - IDE is not supported!

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
make -j $NPROC
```

the ISO and USB images will be written to the build directory.