function(copy_basic TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/os/programs/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/programs/${TARGETFILE}")
    get_filename_component(basic_name ${TARGETFILE} NAME_WE)
    set(OUTNAME_WE "${CMAKE_BINARY_DIR}/iso/programs/${basic_name}")
    add_custom_command(OUTPUT ${OUTNAME_WE}
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/iso/programs" && cp ${FILENAME} ${OUTNAME_WE}
        DEPENDS ${FILENAME})
    add_custom_target(basic_${SOURCEFILE} ALL DEPENDS ${OUTNAME_WE})
    add_dependencies("kernel.bin" basic_${SOURCEFILE})
    add_dependencies(ISO basic_${SOURCEFILE})
endfunction()

function(copy_basic_lib TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/os/programs/libraries/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/programs/libraries/${TARGETFILE}")
    get_filename_component(basic_name ${TARGETFILE} NAME_WE)
    set(OUTNAME_WE "${CMAKE_BINARY_DIR}/iso/programs/libraries/${basic_name}")
    add_custom_command(OUTPUT ${OUTNAME_WE}
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/iso/programs/libraries" && cp ${FILENAME} ${OUTNAME_WE}
        DEPENDS ${FILENAME})
    add_custom_target(basic_${SOURCEFILE} ALL DEPENDS ${OUTNAME_WE})
    add_dependencies("kernel.bin" basic_${SOURCEFILE})
    add_dependencies(ISO basic_${SOURCEFILE})
endfunction()

function(copy_basic_driver TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/os/programs/drivers/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/programs/drivers/${TARGETFILE}")
    get_filename_component(basic_name ${TARGETFILE} NAME_WE)
    set(OUTNAME_WE "${CMAKE_BINARY_DIR}/iso/programs/drivers/${basic_name}")
    add_custom_command(OUTPUT ${OUTNAME_WE}
            COMMAND mkdir -p "${CMAKE_BINARY_DIR}/iso/programs/drivers" && cp ${FILENAME} ${OUTNAME_WE}
            DEPENDS ${FILENAME})
    add_custom_target(basic_${SOURCEFILE} ALL DEPENDS ${OUTNAME_WE})
    add_dependencies("kernel.bin" basic_${SOURCEFILE})
    add_dependencies(ISO basic_${SOURCEFILE})
endfunction()

function(copy_system_keymap SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/os/system/keymaps/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/system/keymaps/${SOURCEFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
            COMMAND mkdir -p "${CMAKE_BINARY_DIR}/iso/system/keymaps" && cp ${FILENAME} ${OUTNAME}
            DEPENDS ${FILENAME})
    add_custom_target(keymap_${SOURCEFILE} ALL DEPENDS ${OUTNAME})
    add_dependencies("kernel.bin" keymap_${SOURCEFILE})
    add_dependencies(ISO keymap_${SOURCEFILE})
endfunction()

function(copy_system_timezone SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/os/system/timezones/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/system/timezones/${SOURCEFILE}")

    add_custom_command(OUTPUT ${OUTNAME}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/iso/system/timezones"
            COMMAND ${CMAKE_COMMAND} -E copy "${FILENAME}" "${OUTNAME}"
            DEPENDS ${FILENAME})

    string(REPLACE "/" "_" TZ_TARGET ${SOURCEFILE})

    add_custom_target(timezone_${TZ_TARGET} ALL DEPENDS ${OUTNAME})
    add_dependencies("kernel.bin" timezone_${TZ_TARGET})
    add_dependencies(ISO timezone_${TZ_TARGET})
endfunction()


function(copy_image TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/os/images/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/images/${TARGETFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/iso/images" && cp ${FILENAME} ${OUTNAME}
        DEPENDS ${FILENAME})
    add_custom_target(img_${SOURCEFILE} ALL DEPENDS ${OUTNAME})
    add_dependencies("kernel.bin" img_${SOURCEFILE})
    add_dependencies(ISO img_${SOURCEFILE})
endfunction()

function(copy_config TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/${TARGETFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/iso/harddisk" && mkdir -p "${CMAKE_BINARY_DIR}/iso/boot" && mkdir -p "${CMAKE_BINARY_DIR}/iso/devices" && mkdir -p "${CMAKE_BINARY_DIR}/iso" && cp ${FILENAME} ${OUTNAME}
        DEPENDS ${FILENAME})
    add_custom_target(config_${TARGETFILE} ALL DEPENDS ${OUTNAME})
    add_dependencies("kernel.bin" config_${TARGETFILE})
    add_dependencies(ISO config_${TARGETFILE})
endfunction()

function(run TARGETFILE)
    set(FILENAME "${CMAKE_BINARY_DIR}/iso/kernel.bin")
    set(OUTNAME "${CMAKE_BINARY_DIR}/${TARGETFILE}")
    # Choose network device based on -DUSE_E1000
    if(USE_E1000)
        set(NET_DEVICE "-device e1000,netdev=netuser")
    else()
        set(NET_DEVICE "-device rtl8139,netdev=netuser")
    endif()
    if (PROFILE_KERNEL)
        set(PROFILE "-serial file:callgrind.out")
    else()
        set(PROFILE "")
    endif()
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND echo "qemu-system-x86_64 \
	-machine q35,accel=kvm \
	-s \
	-monitor stdio \
	-cpu host \
	--enable-kvm \
	-smp 8 \
	-m 4096 \
	-drive id=disk,file=${HARD_DISK_IMAGE},format=raw,if=none \
	-device ahci,id=ahci \
	-device ide-hd,drive=disk,bus=ahci.0 \
	-drive file=rr.iso,media=cdrom,if=none,id=sata-cdrom \
	-device ide-cd,drive=sata-cdrom,bus=ahci.1 \
	-no-shutdown \
	-boot d \
	-vnc 0.0.0.0:2 \
	-debugcon file:debug.log \
	-netdev user,id=netuser,hostfwd=udp::2000-:2000 \
	-object filter-dump,id=dump,netdev=netuser,file=dump.dat \
	${NET_DEVICE} ${PROFILE}" >${OUTNAME} && chmod ugo+x ${OUTNAME}
        DEPENDS ${FILENAME})
    add_custom_target(RUN_${TARGETFILE} ALL DEPENDS ${OUTNAME})
    add_dependencies(RUN_${TARGETFILE} "kernel.bin")
endfunction()

function(debug TARGETFILE)
    set(FILENAME "${CMAKE_BINARY_DIR}/iso/kernel.bin")
    set(OUTNAME "${CMAKE_BINARY_DIR}/${TARGETFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND echo "gdb ${CMAKE_BINARY_DIR}/iso/kernel.bin -ix ${CMAKE_SOURCE_DIR}/.gdbargs" >${OUTNAME} && chmod ugo+x ${OUTNAME}
        DEPENDS ${FILENAME})
    add_custom_target(DEBUG_${TARGETFILE} ALL DEPENDS ${OUTNAME})
    add_dependencies(DEBUG_${TARGETFILE} "kernel.bin")
endfunction()

function(symbols TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_BINARY_DIR}/iso/kernel.bin")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/${TARGETFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND /bin/nm -a "${CMAKE_BINARY_DIR}/iso/kernel.bin" | sort -d > "${CMAKE_BINARY_DIR}/iso/kernel.sym"
        DEPENDS ${FILENAME})
    add_custom_target(SYMBOLS ALL DEPENDS ${OUTNAME})
    add_dependencies(SYMBOLS "kernel.bin")
endfunction()

function(iso TARGETFILE SOURCEFILE)
    set(OUTNAME "${CMAKE_BINARY_DIR}/${TARGETFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND php ../build-boot-image.php && xorriso -as mkisofs --quiet -b limine-cd.bin -joliet -no-emul-boot -boot-load-size 4 -boot-info-table -V "RETROROCKET" --protective-msdos-label "${CMAKE_BINARY_DIR}/iso" -o "${CMAKE_BINARY_DIR}/rr.iso"
        DEPENDS SYMBOLS "kernel.bin" "RUN_run.sh" "DEBUG_debug.sh" ${basic_program_list} ${basic_library_list} ${basic_driver_list} ${KEYMAP_TARGETS} ${TIMEZONE_TARGETS} ${IMAGE_TARGETS})
    add_dependencies(ISO SYMBOLS "kernel.bin" "RUN_run.sh" "DEBUG_debug.sh" "config_limine.cfg")
endfunction()
