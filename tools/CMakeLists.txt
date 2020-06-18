if(NOT KTX_FEATURE_KTX1)
    message(WARNING "KTX_FEATURE_KTX1 is not set -> disabling tools")
    return()
endif()

if(NOT KTX_FEATURE_KTX2)
    message(WARNING "KTX_FEATURE_KTX2 is not set -> disabling tools")
    return()
endif()

if(NOT KTX_FEATURE_WRITE)
    message(WARNING "KTX_FEATURE_WRITE is not set -> disabling tools")
    return()
endif()

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
