#!/bin/bash
rm -rf .iso
mkdir .iso
cp kernel.bin .iso/
mkdir .iso/devices
mkdir .iso/harddisk
cp -v limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-cd-efi.bin .iso/
nm -a kernel.bin | sort -d >.iso/kernel.sym
rm -rf .iso/os
cp -rv os/* .iso/
xorriso -as mkisofs -b limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table -V "RETRO-ROCKET" --protective-msdos-label .iso -o rr.iso
bzip2 -c rr.iso > rr.iso.bz2

