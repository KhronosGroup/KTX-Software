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
            INSTALL_RPATH "@executable_path;/usr/local/lib"
        )
    endif()
endfunction()


add_subdirectory(ktx2check)
add_subdirectory(ktx2ktx2)
add_subdirectory(ktxinfo)
add_subdirectory(ktxsc)
add_subdirectory(toktx)

install(TARGETS
    ktx2check
    ktx2ktx2
    ktxinfo
    ktxsc
    toktx
RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT tools
)
