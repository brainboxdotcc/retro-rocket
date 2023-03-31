#!/bin/bash
rm -rf "$1/.iso"
mkdir "$1/.iso"
cp "$1/kernel.bin" "$1/.iso/"
mkdir "$1/.iso/devices"
mkdir "$1/.iso/harddisk"
cp limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-cd-efi.bin "$1/.iso/"
nm -a "$1/kernel.bin" | sort -d > "$1/.iso/kernel.sym"
rm -rf "$1/.iso/os"
cp -r os/* "$1/.iso/"
xorriso -as mkisofs -b limine-cd.bin -joliet -no-emul-boot -boot-load-size 4 -boot-info-table -V "RETRO-ROCKET" --protective-msdos-label "$1/.iso" -o "$1/rr.iso" 2>/dev/null
cd "$1"
chmod ugo+x *.sh
tar cj "rr.iso" "run.sh" > "rr.iso.bz2"

