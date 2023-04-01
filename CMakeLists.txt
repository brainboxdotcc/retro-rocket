cmake_minimum_required(VERSION 3.22)
project(retrorocket C ASM ASM_NASM)

set(HARD_DISK_IMAGE "../../harddisk0" CACHE STRING "Path to hard disk image when running QEMU")

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_LINK_EXECUTABLE "ld -m elf_x86_64 -nostdlib -no-pie -T ${CMAKE_CURRENT_SOURCE_DIR}/buildtools/linker.ld <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_ASM_FLAGS "-pipe -g -fno-PIC -Wall -nostdlib -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=large -mno-red-zone")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

file(GLOB SOURCES "src/*.c")
file(GLOB ASSEMBLY "asm/*")

set(CMAKE_C_FLAGS "-pipe -g -fno-PIC -Wall -Wno-deprecated-declarations -nostdlib -mno-80387 -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=large -mno-red-zone -Wno-address-of-packed-member")

include_directories("include")
include_directories("limine")

add_executable("kernel.bin" ${SOURCES} ${ASSEMBLY})

add_link_options("-m elf_x86_64 -nostdlib -no-pie -T buildtools/linker.ld")

add_custom_command(TARGET "kernel.bin" POST_BUILD 
  COMMAND "sh" "buildtools/makeiso.sh" "${CMAKE_CURRENT_BINARY_DIR}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Build ISO Image"
)

add_custom_command(TARGET "kernel.bin" POST_BUILD
  COMMAND "echo" "qemu-system-x86_64 \
  		-s \
		-cpu host \
		--enable-kvm \
		-smp 8 \
		-usb \
		-usbdevice mouse \
		-m 4096 \
		-drive id=disk,file=${HARD_DISK_IMAGE},format=raw,if=none \
		-device ahci,id=ahci \
		-device ide-hd,drive=disk,bus=ahci.0 \
		-drive file=rr.iso,media=cdrom,if=none,id=sata-cdrom \
		-device ide-cd,drive=sata-cdrom,bus=ahci.1 \
		-no-reboot \
		-no-shutdown \
		-boot d \
		-vnc 0.0.0.0:2 \
		-debugcon file:debug.log \
		-netdev user,id=netuser,hostfwd=tcp::2000-:2000 \
		-object filter-dump,id=dump,netdev=netuser,file=dump.dat \
		-device rtl8139,netdev=netuser" ">run.sh"
  COMMENT "Create qemu script"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_custom_command(TARGET "kernel.bin" POST_BUILD
  COMMAND "echo" "gdb \
  		kernel.bin \
		-ix ${CMAKE_CURRENT_SOURCE_DIR}/.gdbargs" ">debug.sh"
  COMMENT "Create debug script"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_custom_command(TARGET "kernel.bin" POST_BUILD
  COMMAND "chmod" "ugo+x" "*.sh"
  COMMENT "Set permissions on scripts"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
