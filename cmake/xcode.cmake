# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

# Macro for setting up code signing on targets
macro (set_xcode_code_sign target)
if(APPLE)
    set_target_properties(${target} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${XCODE_CODE_SIGN_IDENTITY}"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${XCODE_DEVELOPMENT_TEAM}"
        XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp"
        XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS $<IF:$<CONFIG:Debug>,YES,NO>
        BUILD_WITH_INSTALL_RPATH ON
    )
    if(IOS)
        set_property (TARGET ${target} PROPERTY XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER ${XCODE_PROVISIONING_PROFILE_SPECIFIER})
    endif()
endif()
endmacro (set_xcode_code_sign)
