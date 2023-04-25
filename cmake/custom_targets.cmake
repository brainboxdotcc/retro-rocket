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

function(copy_font TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/os/fonts/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/fonts/${TARGETFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/iso/fonts" && cp ${FILENAME} ${OUTNAME}
        DEPENDS ${FILENAME})
    add_custom_target(font_${SOURCEFILE} ALL DEPENDS ${OUTNAME})
    add_dependencies("kernel.bin" font_${SOURCEFILE})
    add_dependencies(ISO font_${SOURCEFILE})
endfunction()

function(copy_config TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/${TARGETFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/iso/harddisk" && mkdir -p "${CMAKE_BINARY_DIR}/iso/devices" && mkdir -p "${CMAKE_BINARY_DIR}/iso" && cp ${FILENAME} ${OUTNAME}
        DEPENDS ${FILENAME})
    add_custom_target(config_${TARGETFILE} ALL DEPENDS ${OUTNAME})
    add_dependencies("kernel.bin" config_${TARGETFILE})
    add_dependencies(ISO config_${TARGETFILE})
endfunction()

function(run TARGETFILE)
    set(FILENAME "${CMAKE_BINARY_DIR}/iso/kernel.bin")
    set(OUTNAME "${CMAKE_BINARY_DIR}/${TARGETFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND echo "qemu-system-x86_64 \
	-s \
	-cpu host \
	-trace *msi* \
	--enable-kvm \
	-monitor stdio \
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
	-device rtl8139,netdev=netuser" >${OUTNAME} && chmod ugo+x ${OUTNAME}
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
        COMMAND xorriso -as mkisofs -b limine-cd.bin -joliet -no-emul-boot -boot-load-size 4 -boot-info-table -V "RETROROCKET" --protective-msdos-label "${CMAKE_BINARY_DIR}/iso" -o "${CMAKE_BINARY_DIR}/rr.iso"
        DEPENDS SYMBOLS "kernel.bin" "RUN_run.sh" "DEBUG_debug.sh" ${basic_program_list})
    add_dependencies(ISO SYMBOLS "kernel.bin" "RUN_run.sh" "DEBUG_debug.sh")
endfunction()
