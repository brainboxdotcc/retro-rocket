# Definitions - Toolchain
ASM						:= nasm
ASM_FLAGS				:= -f elf64

CC						:= gcc
CC_FLAGS				:= -m64 -ffreestanding -Wall -mcmodel=large

LD						:= ld
LD_FLAGS				:= -z max-page-size=0x1000
LD_FILE					:= link.ld

# Definitions - Paths and Files
BUILD_BIN				:= kernel.bin
BUILD_OBJ				:= boot.o main.o

TARGET_ISO				:= ../../iso/boot/

# Targets
install: all
	cp $(BUILD_BIN) $(TARGET_ISO)

all: build link

link:
	$(LD) $(LD_FLAGS) -T $(LD_FILE) $(BUILD_OBJ) -o $(BUILD_BIN)

build: $(BUILD_OBJ)

%.o: %.asm
	$(ASM) $(ASM_FLAGS) $< -o $@
	
%.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@
	
clean:
	rm $(BUILD_OBJ) $(BUILD_BIN)