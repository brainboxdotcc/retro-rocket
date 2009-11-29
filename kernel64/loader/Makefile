NASMFLAGS=-felf32
CFLAGS=-m32 -Wall -nostdlib -nostdinc -ffreestanding -nostartfiles -nodefaultlibs
LDFLAGS=-m `perl ../build/getsystem32.pl` -Tlinker.ld

all:
	nasm -felf32 -o loader1.o loader.asm
	gcc -o loader2.o -c loader.c ${CFLAGS}
	ld $(LDFLAGS) -o kernel-loader loader1.o loader2.o
