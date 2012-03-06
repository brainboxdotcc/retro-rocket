LDFLAGS=-m `perl build/getsystem.pl` -T build/linker.ld
CFLAGS=-static-libgcc -pipe -m32 -Wall -nostdlib -nostdinc -ffreestanding -nostartfiles -nodefaultlibs -I ../include
ASOURCES=assembly.o loader.o process.o
CSOURCES=debugger.o interrupts.o keyboard.o multiboot-kernel.o paging.o taskswitch.o video.o ata.o errorhandler.o iso9660.o kmalloc.o memcpy.o kprintf.o string.o timer.o filesystem.o elf.o devfs.o syscall.o ubasic.o tokenizer.o input.o fat32.o pci.o apic.o

all: source link

run: all emulator

clean:
	rm asm/*.o src/*.o

link:
	ld $(LDFLAGS) -o kernel.bin src/*.o asm/*.o `locate libgcc.a | head -n1`
	chmod ugo-x kernel.bin
	perl build/makeiso.pl
	scp sixty-four.iso brain@brainbox.cc:/var/www/tmp/rr/rr.iso &

source:
	make -C asm all
	make -C src all CSOURCES="$(CSOURCES)" CFLAGS="$(CFLAGS)"

emulator:
	qemu -soundhw ac97 -usb -usbdevice mouse -m 32 -hda ../harddisk0 -smp 4 -no-reboot -no-shutdown -cdrom sixty-four.iso -boot d

debug:
	nohup qemu -hda ../harddisk0 -smp 4 -no-reboot -no-shutdown -S -gdb tcp::1234 -cdrom sixty-four.iso -boot d &
	sleep 2
	gdb -command=.gdbargs

remoterun: all
	qemu -hda ../harddisk0 -smp 4 -vnc 0.0.0.0:1 -no-reboot -no-shutdown -cdrom sixty-four.iso -monitor stdio -boot d

remotedebug: all
	nohup qemu -soundhw ac97 -usb -usbdevice mouse -m 32 -hda ../harddisk0 -smp 4 -S -gdb tcp::1234 -no-reboot -no-shutdown -vnc 0.0.0.0:1 -cdrom sixty-four.iso -boot d &
	sleep 2
	gdb -command=.gdbargs

