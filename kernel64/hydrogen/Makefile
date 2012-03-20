#-------------------------------------------------------------------------------
# Definitions - Paths
#-------------------------------------------------------------------------------

BUILD_DIR		:= build
BUILD_MAIN		:= $(BUILD_DIR)/hydrogen.o
BUILD_LINKED	:= $(BUILD_DIR)/hydrogen.bin

SOURCE_DIR		:= src
SOURCE_MAIN		:= $(SOURCE_DIR)/hydrogen.asm
SOURCE_LINK		:= link.ld

ISO				:= boot.iso
ISO_DIR			:= iso
ISO_ELTORITO	:= boot/stage2_eltorito
ISO_HYDROGEN	:= boot/hydrogen.bin

#-------------------------------------------------------------------------------
# Definitions - Tools
#-------------------------------------------------------------------------------

ASM 			:= nasm
ASM_FLAGS 		:= -I./src/ -f elf64

EMU				:= bochs
EMU_FLAGS		:= -q

#-------------------------------------------------------------------------------
# Targets
#-------------------------------------------------------------------------------

# Builds all binaries
.PHONY: all
all:
	mkdir -p $(BUILD_DIR)
	$(ASM) $(ASM_FLAGS) $(SOURCE_MAIN) -o $(BUILD_MAIN)
	
# Links all binaries
link: all
	$(LD) -z max-page-size=4096 -T $(SOURCE_LINK) -o $(BUILD_LINKED) $(BUILD_MAIN)
	objcopy -x $(BUILD_LINKED) $(BUILD_LINKED)
	
# Cleans all binaries
clean:
	rm $(BUILD_LINKED) $(BUILD_MAIN) $(ISO)
	
# Creates an ISO image
iso: link
	cp $(BUILD_LINKED) iso/$(ISO_HYDROGEN)
	mkisofs -R -b $(ISO_ELTORITO) -no-emul-boot -boot-load-size 4 \
	          -boot-info-table -o $(ISO) $(ISO_DIR)
	          
# Runs the ISO image in an emulator
run: iso
	$(EMU) $(EMU_FLAGS)
