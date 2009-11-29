LDFLAGS=-m `perl build/getsystem.pl` -T build/linker.ld
CFLAGS=-pipe -m32 -Wall -nostdlib -nostdinc -ffreestanding -nostartfiles -nodefaultlibs
ASOURCES=loader.o
CSOURCES=kernel.o

all: loader source link

run: all emulator

clean:
	rm asm/*.o src/*.o loader/*.o

link:
	-ld $(LDFLAGS) -o kernel.bin src/*.o asm/*.o
	-chmod ugo-x kernel.bin
	perl build/makeiso.pl

loader:
	make -C loader all

source:
	-make -C asm all
	-make -C src all CSOURCES="$(CSOURCES)" CFLAGS="$(CFLAGS)"

emulator:
	qemu -no-reboot -no-shutdown -cdrom sixty-four.iso

debug: all
	nohup qemu -no-reboot -no-shutdown -S -gdb tcp::1234 -cdrom sixty-four.iso &
	sleep 2
	gdb -command=.gdbargs

remoterun: all
	qemu -vnc 0.0.0.0:1 -no-reboot -no-shutdown -cdrom sixty-four.iso

remotedebug: all
	nohup qemu -S -gdb tcp::1234 -no-reboot -no-shutdown -vnc 0.0.0.0:1 -cdrom sixty-four.iso &
	sleep 2
	gdb -command=.gdbargs

