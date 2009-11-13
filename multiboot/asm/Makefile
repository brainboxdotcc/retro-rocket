NASMFLAGS=-felf
CFLAGS=-m32 -Wall -nostdlib -nostdinc -ffreestanding -nostartfiles -nodefaultlibs

all:
	nasm -felf -o loader.o loader.asm
	gcc -o assembly.o -c assembly.s ${CFLAGS}
	nasm -felf -o process.o process.asm

