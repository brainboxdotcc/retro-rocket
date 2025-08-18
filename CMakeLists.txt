cmake_minimum_required(VERSION 3.22)
project(retrorocket C ASM ASM_NASM)

option(USE_E1000 "Use the e1000 NIC driver in QEMU instead of rtl8139" ON)
option(PROFILE_KERNEL "Profile the kernel, and output callgrind compatible file via COM1" OFF)

set(HARD_DISK_IMAGE "../../harddisk0" CACHE STRING "Path to hard disk image when running QEMU")

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_LINK_EXECUTABLE "ld -m elf_x86_64 -nostdlib -no-pie -T ${CMAKE_CURRENT_SOURCE_DIR}/buildtools/linker.ld <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_ASM_FLAGS "-pipe -g -fno-PIC -Wall -Wextra -Wno-unused-parameter -Wno-int-to-pointer-cast -nostdlib -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=large -mno-red-zone")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

include("${CMAKE_SOURCE_DIR}/cmake/custom_targets.cmake")

file(GLOB_RECURSE SOURCES "src/*.c")
file(GLOB ASSEMBLY "asm/*")

set(CMAKE_C_FLAGS "-pipe -g -mpopcnt -fno-PIC -std=c17 -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter -Wno-pointer-to-int-cast -Wno-discarded-qualifiers -Wno-int-conversion -Wno-int-to-pointer-cast -nostdlib -mno-mmx -mno-3dnow -ffreestanding -nostartfiles -nodefaultlibs -mcmodel=kernel -mno-red-zone -Wno-address-of-packed-member")

if(PROFILE_KERNEL)
    add_compile_definitions(PROFILE_KERNEL)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finstrument-functions")
    set_source_files_properties(
            src/profiler.c src/serial.c
            PROPERTIES COMPILE_FLAGS "-fno-instrument-functions"
    )
endif()

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

file(GLOB image_list ${CMAKE_SOURCE_DIR}/os/images/*)
set(IMAGE_TARGETS "")
foreach (image ${image_list})
    get_filename_component(image_name ${image} NAME)
    copy_image(${image_name} ${image_name})
    list(APPEND IMAGE_TARGETS "img_${image_name}")
endforeach()

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
set(KEYMAP_TARGETS "")
foreach (keymap ${keymap_list})
    get_filename_component(keymap_name ${keymap} NAME)
    copy_system_keymap(${keymap_name} ${keymap_name})
    list(APPEND KEYMAP_TARGETS "keymap_${keymap_name}")
endforeach()
set(TIMEZONE_TARGETS "")
file(GLOB_RECURSE timezone_list RELATIVE ${CMAKE_SOURCE_DIR}/os/system/timezones ${CMAKE_SOURCE_DIR}/os/system/timezones/*)
foreach (timezone ${timezone_list})
    copy_system_timezone(${timezone})
    string(REPLACE "/" "_" TZ_NAME ${timezone})
    list(APPEND TIMEZONE_TARGETS "timezone_${TZ_NAME}")
endforeach()


iso("rr.iso" "kernel.sym")
