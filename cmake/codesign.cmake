# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

# Macro for setting up code signing on targets
macro (set_code_sign target)
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

  if (WIN32 AND WIN_CODE_SIGN_IDENTITY)
    find_package(signtool REQUIRED)

    if (signtool_EXECUTABLE)
      configure_sign_params()
      add_custom_command( TARGET ${target}
       POST_BUILD
       COMMAND ${signtool_EXECUTABLE} sign ${SIGN_PARAMS} $<TARGET_FILE:${target}>
       VERBATIM
      )
    endif()
  endif()
endmacro (set_code_sign)

function(configure_sign_params)
  if (NOT SIGN_PARAMS)
    # Default to looking for cert. in user's store but let user tell us
    # to look in Local Computer store. See comment at begin_build phase
    # in .appveyor.yml for the reason. User store is preferred because
    # importing the cert. does not need admin elevation.
    if (WIN_CS_CERT_SEARCH_MACHINE_STORE)
      set(store "/sm")
    endif()
    set(SIGN_PARAMS ${store} /fd sha256 /n "${WIN_CODE_SIGN_IDENTITY}"
        /tr http://ts.ssl.com /td sha256
        /d KTX-Software /du https://github.com/KhronosGroup/KTX-Software
        PARENT_SCOPE)
  endif()
endfunction()

function(set_nsis_installer_codesign_cmd)
  if (WIN32 AND WIN_CODE_SIGN_IDENTITY)
    # To make calls to the above macro and this order independent ...
    find_package(signtool REQUIRED)
    if (signtool_EXECUTABLE)
      configure_sign_params()
      # CPACK_NSIS_FINALIZE_CMD is a variable whose value is to be substituted
      # into the !finalize and !uninstfinalize commands in
      # cmake/modules/NSIS.template.in. This variable is ours. It is not a
      # standard CPACK variable. The name MUST start with CPACK otherwise
      # it will not be defined when cpack runs its configure_file step.
      foreach(param IN LISTS SIGN_PARAMS)
        # Quote the parameters because at least one of them,
        # WIN_CODE_SIGN_IDENTITY, has spaces. It is easier to quote
        # all of them than determine which have spaces.
        #
        # Insane escaping is needed due to the 2-step process used to
        # configure the final output. First cpack creates CPackConfig.cmake
        # in which the value set here appears, inside quotes, as the
        # argument to a cmake `set` command. That variable's value
        # is then substituted into the output.
        string(APPEND NSIS_SIGN_PARAMS "\\\"${param}\\\" ")
      endforeach()

      # Note 1: cpack/NSIS does not show any output when running signtool,
      # whether it succeeds or fails.
      #
      # Note 2: Do not move the %1 to NSIS.template.in. We need an empty
      # command there when we aren't signing. %1 is replaced by the name
      # of the installer or uninstaller during NSIS compilation.
      set(CPACK_NSIS_FINALIZE_CMD "\\\"${signtool_EXECUTABLE}\\\" sign ${NSIS_SIGN_PARAMS} %1"
        PARENT_SCOPE
      )
      unset(NSIS_SIGN_PARAMS)
    endif()
  endif()
endfunction()
