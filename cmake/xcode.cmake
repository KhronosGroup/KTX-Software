# Macro for setting up code signing on targets
macro (set_xcode_code_sign target)
set_target_properties(${target} PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${XCODE_CODE_SIGN_IDENTITY}"
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${XCODE_DEVELOPMENT_TEAM}"
)

if(XCODE_CODE_SIGN_FOR_RELEASE)
    set_target_properties(${target} PROPERTIES
        BUILD_WITH_INSTALL_RPATH ON
    )
endif()

endmacro (set_xcode_code_sign)
