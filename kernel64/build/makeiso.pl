#!/usr/bin/perl

system("rm -rf .iso");
mkdir(".iso");
system("cp kernel.bin .iso/");
mkdir(".iso/boot");
mkdir(".iso/boot");
mkdir(".iso/devices");
mkdir(".iso/harddisk");
system("cp grub/* .iso/boot/");
system("nm -a kernel.bin | sort -d >.iso/kernel.sym");
system("rm -rf .iso/os");
system("cp -rv os/* .iso/");
chdir(".iso");
system("mkisofs -J -R -V \"RETRO-ROCKET\" -b boot/stage2_eltorito -o ../sixty-four.iso -no-emul-boot -boot-load-size 4 -boot-info-table .");
chdir("..");
system("bzip2 -c sixty-four.iso > sixty-four.iso.bz2");

