#!/usr/bin/perl

system("rm -rf .iso");
mkdir(".iso");
system("cp kernel.bin .iso/");
mkdir(".iso/boot");
mkdir(".iso/boot");
mkdir(".iso/devices");
system("cp grub/* .iso/boot/");
system("nm -a kernel.bin | sort -d >.iso/kernel.sym");
chdir(".iso");
system("mkisofs -R -V \"RETRO-ROCKET\" -b boot/stage2_eltorito -o ../sixty-four.iso -no-emul-boot -boot-load-size 4 -boot-info-table .");
chdir("..");
system("bzip2 -c sixty-four.iso > sixty-four.iso.bz2");

