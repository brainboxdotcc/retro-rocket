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

function(copy_system_web SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/os/system/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/iso/system/${SOURCEFILE}")

    add_custom_command(OUTPUT ${OUTNAME}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/iso/system"
            COMMAND ${CMAKE_COMMAND} -E copy "${FILENAME}" "${OUTNAME}"
            DEPENDS ${FILENAME})

    string(REPLACE "/" "_" WEB_TARGET ${SOURCEFILE})

    add_custom_target(web_${WEB_TARGET} ALL DEPENDS ${OUTNAME})
    add_dependencies("kernel.bin" web_${WEB_TARGET})
    add_dependencies(ISO web_${WEB_TARGET})
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
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/iso/harddisk" && mkdir -p "${CMAKE_BINARY_DIR}/iso/ramdisk" && mkdir -p "${CMAKE_BINARY_DIR}/iso/boot" && mkdir -p "${CMAKE_BINARY_DIR}/iso/devices" && mkdir -p "${CMAKE_BINARY_DIR}/iso" && cp ${FILENAME} ${OUTNAME}
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

    # ---- Audio detection (Linux only) ----
    set(AUDIO_OPTS "")
    if(UNIX AND NOT APPLE)
        # Try the modern help first. Some older QEMU builds return non-zero and print an error.
        execute_process(
                COMMAND qemu-system-x86_64 -audiodev help
                OUTPUT_VARIABLE QEMU_AD_OUT
                ERROR_VARIABLE QEMU_AD_ERR
                RESULT_VARIABLE QEMU_AD_RC
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE
        )
        # Combine both streams for robust matching
        set(QEMU_AD_TEXT "${QEMU_AD_OUT}\n${QEMU_AD_ERR}")

        if(QEMU_AD_RC EQUAL 0 OR QEMU_AD_TEXT MATCHES "Available audio drivers")
            # Prefer PipeWire if QEMU lists it and host has it; else fall back to PulseAudio
            set(_BACKEND "")
            if(QEMU_AD_TEXT MATCHES "(^|[ \n\r\t])pipewire([ \n\r\t]|$)")
                # Check host has PipeWire
                execute_process(COMMAND pipewire --version RESULT_VARIABLE PW_RC OUTPUT_QUIET ERROR_QUIET)
                if(PW_RC EQUAL 0)
                    set(_BACKEND "pipewire")
                endif()
            endif()
            if(_BACKEND STREQUAL "")
                if(QEMU_AD_TEXT MATCHES "(^|[ \n\r\t])pa([ \n\r\t]|$)")
                    # Check host has PulseAudio (or PipeWire's Pulse server)
                    execute_process(COMMAND pactl --version RESULT_VARIABLE PA_RC OUTPUT_QUIET ERROR_QUIET)
                    if(PA_RC EQUAL 0)
                        set(_BACKEND "pa")
                    endif()
                endif()
            endif()

            if(_BACKEND STREQUAL "pipewire")
                set(AUDIO_OPTS "-audiodev pipewire,id=snd0 -device AC97,audiodev=snd0")
            elseif(_BACKEND STREQUAL "pa")
                # If you strictly only want PipeWire, comment this line out to skip Pulse fallback.
                set(AUDIO_OPTS "-audiodev pa,id=snd0 -device AC97,audiodev=snd0")
            endif()
        endif()
    endif()
    # ---- end audio detection ----

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
	-netdev user,id=netuser,hostfwd=tcp::2000-:2000,hostfwd=tcp::2080-:80 \
	-object filter-dump,id=dump,netdev=netuser,file=dump.dat \
	${NET_DEVICE} ${PROFILE} ${AUDIO_OPTS}" >${OUTNAME} && chmod ugo+x ${OUTNAME}
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
        COMMAND php ../build-boot-image.php && xorriso -as mkisofs --quiet -b limine-bios-cd.bin -joliet -no-emul-boot -boot-load-size 4 -boot-info-table -V "RETROROCKET" --protective-msdos-label "${CMAKE_BINARY_DIR}/iso" -o "${CMAKE_BINARY_DIR}/rr.iso"
        DEPENDS SYMBOLS "kernel.bin" "RUN_run.sh" "DEBUG_debug.sh" ${basic_program_list} ${basic_library_list} ${basic_driver_list} ${WEB_TARGETS} ${CONFIG_TARGETS} ${IMAGE_TARGETS} ${MODULE_TARGETS})
    add_dependencies(ISO SYMBOLS "kernel.bin" "RUN_run.sh" "DEBUG_debug.sh" "config_limine.conf")
endfunction()
