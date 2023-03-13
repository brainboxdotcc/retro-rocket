#!/usr/bin/perl

system("rm -rf .iso");
mkdir(".iso");
system("cp kernel.bin .iso/");
mkdir(".iso/boot");
mkdir(".iso/boot");
mkdir(".iso/devices");
mkdir(".iso/harddisk");
#system("cp grub/* .iso/boot/");
system("cp -v limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-cd-efi.bin .iso/");
system("nm -a kernel.bin | sort -d >.iso/kernel.sym");
system("rm -rf .iso/os");
system("cp -rv os/* .iso/");
#chdir(".iso");
system("xorriso -as mkisofs -b limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --protective-msdos-label .iso -o sixty-four.iso");
#system("mkisofs -J -R -V \"RETRO-ROCKET\" -b limine-cd.bin -o ../sixty-four.iso -no-emul-boot -boot-load-size 4 -boot-info-table .");
#chdir("..");
system("bzip2 -c sixty-four.iso > sixty-four.iso.bz2");

