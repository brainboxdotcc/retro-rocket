cmake_minimum_required(VERSION 3.22)
project(retrorocket C ASM ASM_NASM)

set(HARD_DISK_IMAGE "../../harddisk0" CACHE STRING "Path to hard disk image when running QEMU")

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_LINK_EXECUTABLE "ld -m elf_x86_64 -nostdlib -no-pie -T ${CMAKE_CURRENT_SOURCE_DIR}/buildtools/linker.ld <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_ASM_FLAGS "-pipe -g -fno-PIC -Wall -Wextra -Wno-unused-parameter -nostdlib -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=large -mno-red-zone")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

include("${CMAKE_SOURCE_DIR}/cmake/custom_targets.cmake")

file(GLOB SOURCES "src/*.c")
file(GLOB ASSEMBLY "asm/*")

set(CMAKE_C_FLAGS "-pipe -g -fno-PIC -std=c17 -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter -nostdlib -mno-mmx -mno-3dnow -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=kernel -mno-red-zone -Wno-address-of-packed-member")

include_directories("include")
include_directories("limine")

add_executable("kernel.bin" ${SOURCES} ${ASSEMBLY})

add_link_options("-m elf_x86_64 -nostdlib -no-pie -T buildtools/linker.ld")

add_custom_command(TARGET "kernel.bin" POST_BUILD 
  COMMAND "sh" "buildtools/makeiso.sh" "${CMAKE_CURRENT_BINARY_DIR}" "${HARD_DISK_IMAGE}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Build ISO image and scripts"
)

copy_config(limine.cfg limine.cfg)
copy_config(limine.sys limine/limine.sys)
copy_config(limine-cd.bin limine/limine-cd.bin)
copy_config(imine-efi.bin limine/limine-cd-efi.bin)

copy_font(system.f08 system.f08)
copy_font(system.f16 system.f16)
copy_font(system.f20 system.f20)

file(GLOB basic_program_list ${CMAKE_SOURCE_DIR}/os/programs/*.rrbasic)
foreach (basic_program ${basic_program_list})
    get_filename_component(basic_name ${basic_program} NAME)
    copy_basic(${basic_name} ${basic_name})
endforeach()
