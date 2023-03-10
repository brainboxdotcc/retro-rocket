#!/usr/bin/perl

system("rm -rf .iso");
mkdir(".iso");
system("cp kernel.bin .iso/");
mkdir(".iso/boot");
mkdir(".iso/boot/grub");
mkdir(".iso/devices");
system("cp grub/iso9660_stage1_5 .iso/boot/grub/");
system("cp grub/stage* .iso/boot/grub/");
system("cp grub/menu.lst .iso/boot/grub/");
system("nm -a kernel.bin | sort -d >.iso/kernel.sym");
system("rm -rf .iso/os");
system("cp -rv os/* .iso/");
chdir(".iso");
system("genisoimage -J -R -V \"SIXTY-FOUR\" -b boot/grub/iso9660_stage1_5 -o ../sixty-four.iso -no-emul-boot -boot-load-size 4 -boot-info-table .");
chdir("..");
system("bzip2 -c sixty-four.iso > sixty-four.iso.bz2");

