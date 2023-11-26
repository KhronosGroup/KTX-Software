# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

if(NOT KTX_FEATURE_KTX1)
    message(WARNING "KTX_FEATURE_KTX1 is not set -> disabling tools")
    return()
endif()

if(NOT KTX_FEATURE_KTX2)
    message(WARNING "KTX_FEATURE_KTX2 is not set -> disabling tools")
    return()
endif()

function(set_tool_properties tool_target)
    if(APPLE)
        set_target_properties(${tool_target} PROPERTIES
            XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
            # Creates an LC_RPATH entry in the Mac-O binary for each
            # item in this list. When searching for libraries whose
            # install name starts with @rpath, as libktx's does, dyld
            # searches each LC_RPATH in the order given here.
            #
            # Check the LC_RPATH entries with
            # - otool -l <file> | grep -A 3 LC_RPATH
            #
            # TODO: Consider adding @executable_path/../lib.
            INSTALL_RPATH "@executable_path;/usr/local/lib"
            CXX_VISIBILITY_PRESET ${STATIC_APP_LIB_SYMBOL_VISIBILITY}
        )
    elseif(LINUX)
        set_target_properties(${tool_target} PROPERTIES
            # With modern tools sets DT_RUNPATH not the deprecated
            # DT_RPATH in ELF binaries.  ld.so searches for libraries
            # as follows:
            # - LD_LIBRARY_PATH
            # - RUNPATH
            # - Directories given in /etc/ld.so.conf.
            #   /usr/local/lib is listed there.
            # - Default path: /lib;/usr/lib.
            # $ORIGIN is equivalent to @executable_path.
            #
            # Check DT_RUNPATH with one of
            # - readelf -d <file> | head -20
            # - objdump -x <file> | grep 'R.*PATH'
            INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib"
        )
    endif()
endfunction()

add_subdirectory(imageio)
add_subdirectory(ktx)
add_subdirectory(ktx2check)
add_subdirectory(ktx2ktx2)
add_subdirectory(ktxinfo)
add_subdirectory(ktxsc)
add_subdirectory(toktx)

install(TARGETS
    ktxtools

    ktx2check
    ktx2ktx2
    ktxinfo
    ktxsc
    toktx
RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT tools
)
