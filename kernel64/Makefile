CC=gcc
LD=ld
AS=as
GAS=gas
LDFLAGS=-m elf_x86_64 -nostdlib -no-pie -T build/linker.ld
CFLAGS=-pipe -g -fno-PIC -Wall -nostdlib -nostdinc -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=large -mno-red-zone -I ../include
ASOURCES=boot.o loader.o spinlock.o
CSOURCES=acpi.o kernel.o printf.o string.o video.o memcpy.o interrupt.o paging.o debugger.o kmalloc.o timer.o ioapic.o ata.o filesystem.o iso9660.o devfs.o fat32.o errorhandler.o idt.o keyboard.o input.o pci.o apic.o lapic_timer.o taskswitch.o tokenizer.o ubasic.o

all: source link

run: all emulator

clean:
	rm asm/*.o src/*.o

link:
	$(LD) $(LDFLAGS) -o kernel.bin src/*.o asm/*.o
	chmod ugo-x kernel.bin
	perl build/makeiso.pl

source:
	make -C asm all CC="$(CC)" ASOURCES="$(ASOURCES)" CFLAGS="$(CFLAGS)"
	make -C src all CC="$(CC)" CSOURCES="$(CSOURCES)" CFLAGS="$(CFLAGS)"

remoterun: all
	-qemu-system-x86_64 -usb -usbdevice mouse -m 4096 -hda ../harddisk0 -no-reboot -no-shutdown -cdrom sixty-four.iso -boot d -vnc 0.0.0.0:2

harddisk:
	qemu-img create harddisk.img 1G

