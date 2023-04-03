cmake_minimum_required(VERSION 3.22)
project(retrorocket C ASM ASM_NASM)

set(HARD_DISK_IMAGE "../../harddisk0" CACHE STRING "Path to hard disk image when running QEMU")

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_LINK_EXECUTABLE "ld -m elf_x86_64 -nostdlib -no-pie -T ${CMAKE_CURRENT_SOURCE_DIR}/buildtools/linker.ld <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_ASM_FLAGS "-pipe -g -fno-PIC -Wall -Wextra -Wno-unused-parameter -nostdlib -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=large -mno-red-zone")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

file(GLOB SOURCES "src/*.c")
file(GLOB ASSEMBLY "asm/*")

set(CMAKE_C_FLAGS "-pipe -g -fno-PIC -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter -nostdlib -mno-mmx -mno-3dnow -mno-sse -mno-sse2 -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=large -mno-red-zone -Wno-address-of-packed-member")

include_directories("include")
include_directories("limine")

add_executable("kernel.bin" ${SOURCES} ${ASSEMBLY})

add_link_options("-m elf_x86_64 -nostdlib -no-pie -T buildtools/linker.ld")

add_custom_command(TARGET "kernel.bin" POST_BUILD 
  COMMAND "sh" "buildtools/makeiso.sh" "${CMAKE_CURRENT_BINARY_DIR}" "${HARD_DISK_IMAGE}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Build ISO image and scripts"
)
