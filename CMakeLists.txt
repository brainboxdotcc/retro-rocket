cmake_minimum_required(VERSION 3.22)
project(retrorocket C ASM ASM_NASM)

option(USE_E1000 "Use the e1000 NIC driver in QEMU instead of rtl8139" ON)

set(HARD_DISK_IMAGE "../../harddisk0" CACHE STRING "Path to hard disk image when running QEMU")

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_LINK_EXECUTABLE "ld -m elf_x86_64 -nostdlib -no-pie -T ${CMAKE_CURRENT_SOURCE_DIR}/buildtools/linker.ld <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_ASM_FLAGS "-pipe -g -fno-PIC -Wall -Wextra -Wno-unused-parameter -Wno-int-to-pointer-cast -nostdlib -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=large -mno-red-zone")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

include("${CMAKE_SOURCE_DIR}/cmake/custom_targets.cmake")

file(GLOB_RECURSE SOURCES "src/*.c")
file(GLOB ASSEMBLY "asm/*")

set(CMAKE_C_FLAGS "-pipe -g -fno-PIC -std=c17 -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter -Wno-pointer-to-int-cast -Wno-discarded-qualifiers -Wno-int-conversion -Wno-int-to-pointer-cast -nostdlib -mno-mmx -mno-3dnow -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=kernel -mno-red-zone -Wno-address-of-packed-member")

include_directories("include")
include_directories("limine")
include_directories("uacpi/include")
include_directories(${UACPI_INCLUDES})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/iso)

include(uacpi/uacpi.cmake)

add_executable(kernel.bin ${SOURCES} ${ASSEMBLY})
target_sources(kernel.bin PRIVATE ${UACPI_SOURCES})
target_include_directories(kernel.bin PRIVATE ${UACPI_INCLUDES})

add_link_options("-m elf_x86_64 -nostdlib -no-pie -T buildtools/linker.ld")

add_custom_target(ISO ALL DEPENDS ${CMAKE_BINARY_DIR}/rr.iso)

copy_config(limine.cfg limine.cfg)
copy_config(limine.sys limine/limine.sys)
copy_config(limine-cd.bin limine/limine-cd.bin)
copy_config(limine-efi.bin limine/limine-cd-efi.bin)

copy_font(system.f08 system.f08)
copy_font(system.f16 system.f16)
copy_font(system.f20 system.f20)

copy_image(brainbox.png brainbox.png)
copy_image(brainbox.gif brainbox.gif)
copy_image(computer.jpg computer.jpg)

symbols("kernel.sym" "kernel.bin")

run("run.sh")
debug("debug.sh")

file(GLOB basic_program_list ${CMAKE_SOURCE_DIR}/os/programs/*.rrbasic)
foreach (basic_program ${basic_program_list})
    get_filename_component(basic_name ${basic_program} NAME)
    copy_basic(${basic_name} ${basic_name})
endforeach()
file(GLOB basic_library_list ${CMAKE_SOURCE_DIR}/os/programs/libraries/*.rrbasic)
foreach (basic_library ${basic_library_list})
    get_filename_component(basic_name ${basic_library} NAME)
    copy_basic_lib(${basic_name} ${basic_name})
endforeach()
file(GLOB keymap_list ${CMAKE_SOURCE_DIR}/os/system/keymaps/*.keymap)
foreach (keymap ${keymap_list})
    get_filename_component(keymap_name ${keymap} NAME)
    copy_system_keymap(${keymap_name} ${keymap_name})
endforeach()

iso("rr.iso" "kernel.sym")
