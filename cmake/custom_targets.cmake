function(copy_basic TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/os/programs/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/.iso/programs/${TARGETFILE}")
    get_filename_component(basic_name ${TARGETFILE} NAME_WE)
    set(OUTNAME_WE "${CMAKE_BINARY_DIR}/.iso/programs/${basic_name}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/.iso/programs" && cp -v ${FILENAME} ${OUTNAME_WE}
        DEPENDS ${FILENAME})
    add_custom_target(basic_${SOURCEFILE} ALL DEPENDS ${OUTNAME})
    add_dependencies("kernel.bin" basic_${SOURCEFILE})
endfunction()

function(copy_font TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/os/fonts/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/.iso/fonts/${TARGETFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/.iso/fonts" && cp -v ${FILENAME} ${OUTNAME}
        DEPENDS ${FILENAME})
    add_custom_target(font_${SOURCEFILE} ALL DEPENDS ${OUTNAME})
    add_dependencies("kernel.bin" font_${SOURCEFILE})
endfunction()

function(copy_config TARGETFILE SOURCEFILE)
    set(FILENAME "${CMAKE_SOURCE_DIR}/${SOURCEFILE}")
    set(OUTNAME "${CMAKE_BINARY_DIR}/.iso/${TARGETFILE}")
    add_custom_command(OUTPUT ${OUTNAME}
        COMMAND mkdir -p "${CMAKE_BINARY_DIR}/.iso" && cp -v ${FILENAME} ${OUTNAME}
        DEPENDS ${FILENAME})
    add_custom_target(config_${TARGETFILE} ALL DEPENDS ${OUTNAME})
    add_dependencies("kernel.bin" config_${TARGETFILE})
endfunction()

