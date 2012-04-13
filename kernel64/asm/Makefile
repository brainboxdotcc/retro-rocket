all:
	nasm -f elf64 boot.asm
	nasm -f elf64 spinlock.asm
	$(CC) -c loader.S $(CFLAGS)

