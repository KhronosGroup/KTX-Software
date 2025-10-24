# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

# Config options for code signing

if(APPLE)
    # Signing
    set(XCODE_CODE_SIGN_IDENTITY "Development" CACHE STRING "Xcode code sign ID")
    set(XCODE_DEVELOPMENT_TEAM "" CACHE STRING "Xcode development team ID")
    set(PRODUCTBUILD_IDENTITY_NAME "" CACHE STRING "productbuild identity name")
    set(PRODUCTBUILD_KEYCHAIN_PATH "" CACHE FILEPATH "pkgbuild keychain file")
    if(APPLE_LOCKED_OS)
        set(XCODE_PROVISIONING_PROFILE_SPECIFIER "" CACHE STRING "Xcode provisioning profile specifier")
    endif()

    # Deployment
    # When changing the target you must also edit the triplet files in
    # vcpkg-triplets to reflect the new target.
    if(APPLE_MAC_OS)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS Deployment Target")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "iOS/tvOS Deployment Target")
        set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH NO)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "1.0" CACHE STRING "visionOS Deployment Target")
    endif()
endif()

if(WIN32)
    if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.18.0")
        include(KtxDependentVariable)
        # Signing
        set(CODE_SIGN_KEY_VAULT "" CACHE STRING "Name of the vault to search for signing certificate. Enables related code signing variables when set.")
        set_property(CACHE CODE_SIGN_KEY_VAULT
            PROPERTY STRINGS "" Azure Machine User)

        # Common signing variables
        KTX_DEPENDENT_VARIABLE( CODE_SIGN_TIMESTAMP_URL STRING
          "URL of timestamp server for code signing. Signed code should be timestamped so the signature will remain valid even after the certificate expires."
          ""
          "CODE_SIGN_KEY_VAULT"
          ""
        )
        # Variables for signing with local certificate.
        KTX_DEPENDENT_VARIABLE( LOCAL_KEY_VAULT_SIGNING_IDENTITY STRING
            "Subject Name of Windows code signing certificate. Displayed in 'Issued To' field of cert{lm,mgr}. Overriden by LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Machine OR ${CODE_SIGN_KEY_VAULT} MATCHES User"
            ""
        )
        KTX_DEPENDENT_VARIABLE( LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT STRING
            "Thumbprint of the certificate to use. Use this instead of LOCAL_KEY_VAULT_SIGNING_IDENTITY when you have multiple certificates with the same identity. Overrides LOCAL_KEY_VAULT_SIGNING_IDENTITY."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Machine OR ${CODE_SIGN_KEY_VAULT} MATCHES User"
            ""
        )

        # Variables for signing with certificate from Azure
        KTX_DEPENDENT_VARIABLE( AZURE_KEY_VAULT_URL STRING
            "The URL of your Azure key vault."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Azure"
            ""
        )
        KTX_DEPENDENT_VARIABLE( AZURE_KEY_VAULT_CERTIFICATE STRING
            "The name of the certificate in Azure Key Vault."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Azure"
            ""
        )
        KTX_DEPENDENT_VARIABLE( AZURE_KEY_VAULT_CLIENT_ID STRING
            "The id of an application (Client) registered with Azure that has permission to access the certificate."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Azure"
            ""
        )
        KTX_DEPENDENT_VARIABLE( AZURE_KEY_VAULT_TENANT_ID STRING
            "The id of the Azure Active Directory (Tenant) holding the Client."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Azure"
            ""
        )
        KTX_DEPENDENT_VARIABLE( AZURE_KEY_VAULT_CLIENT_SECRET STRING
            "The secret to authenticate access to the Client."
            ""
            "CODE_SIGN_KEY_VAULT;${CODE_SIGN_KEY_VAULT} MATCHES Azure"
            ""
        )
    else()
        # KTX_DEPENDENT_VARIABLE won't work. Force disable signing.
        unset(CODE_SIGN_KEY_VAULT CACHE)
    endif()
endif()

# Macro for setting up code signing on targets
macro (set_code_sign target)
  if(APPLE)
    set_target_properties(${target} PROPERTIES
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${XCODE_CODE_SIGN_IDENTITY}"
      XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${XCODE_DEVELOPMENT_TEAM}"
      XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp"
      XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS $<IF:$<CONFIG:Debug>,YES,NO>
    )
    if(IOS)
      set(set_pps FALSE)
      if(${ARGC} EQUAL 1)
        set(set_pps TRUE)
      elseif (NOT ${ARGV1} STREQUAL "NOPPS")
        set(set_pps TRUE)
      endif()
      if (${set_pps})
        set_property(TARGET ${target}
           PROPERTY XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER ${XCODE_PROVISIONING_PROFILE_SPECIFIER}
        )
      endif()
      unset(set_pps)
    endif()
  endif()

  if(WIN32 AND CODE_SIGN_KEY_VAULT)
    configure_windows_sign_params()
    if(SIGN_PARAMS)
      add_custom_command( TARGET ${target}
        POST_BUILD
        COMMAND ${signtool_EXECUTABLE} sign ${SIGN_PARAMS} $<TARGET_FILE:${target}>
        VERBATIM
      )
    endif()
  endif()
endmacro (set_code_sign)

function(configure_windows_sign_params)
  if(NOT SIGN_PARAMS)
    if(CODE_SIGN_KEY_VAULT STREQUAL "Azure")
      # Use EV certificate in Azure key vault
      find_program(signtool_EXECUTABLE azuresigntool REQUIRED)
      if(signtool_EXECUTABLE)
        configure_azuresigntool_params()
      endif()
    else()
      # Use standard OV certificate in local certificate store.
      find_package(signtool REQUIRED)
      if(signtool_EXECUTABLE)
        configure_signtool_params()
      endif()
    endif()
    set(SIGN_PARAMS ${SIGN_PARAMS} PARENT_SCOPE)
  endif()
endfunction()

function(configure_azuresigntool_params)
  if(NOT SIGN_PARAMS)
    foreach(param AZURE_KEY_VAULT_CERTIFICATE AZURE_KEY_VAULT_URL AZURE_KEY_VAULT_CLIENT_ID AZURE_KEY_VAULT_CLIENT_SECRET AZURE_KEY_VAULT_TENANT_ID)
      if(NOT ${param})
        message(SEND_ERROR "CODE_SIGN_KEY_VAULT set to \"Azure\" but necessary parameter ${param} is not set.")
      endif()
    endforeach()
    configure_common_params()
    set(SIGN_PARAMS ${SIGN_PARAMS}
        --azure-key-vault-url ${AZURE_KEY_VAULT_URL}
        --azure-key-vault-client-id ${AZURE_KEY_VAULT_CLIENT_ID}
        --azure-key-vault-client-secret ${AZURE_KEY_VAULT_CLIENT_SECRET}
        --azure-key-vault-tenant-id ${AZURE_KEY_VAULT_TENANT_ID}
        --azure-key-vault-certificate ${AZURE_KEY_VAULT_CERTIFICATE}
        #--verbose # Include additional output.
        #--quiet   # Do not print any output to the console.
        PARENT_SCOPE)
  endif()
endfunction()

function(configure_signtool_params)
  if(NOT SIGN_PARAMS)
    # User store is preferred because importing the cert. does not need
    # admin elevation. However problems were encountered on Appveyor,
    # see comment at begin_build phase in .appveyor.yml, so use of local
    # machine store is also supported.
    if(CODE_SIGN_KEY_VAULT STREQUAL Machine)
      # Search in local machine certificate store
      set(store "/sm")
    elseif(CODE_SIGN_KEY_VAULT STREQUAL User)
      # Search in user's certificate store.
      unset(store)
    else()
      message(FATAL_ERROR "Unrecognized CODE_SIGN_KEY_VAULT value \"${CODE_SIGN_KEY_VAULT}\"")
    endif()
    if(LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT)
      set(certopt /sha1)
      set(certid ${LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT})
    elseif(LOCAL_KEY_VAULT_SIGNING_IDENTITY)
      set(certopt /n)
      set(certid ${LOCAL_KEY_VAULT_SIGNING_IDENTITY})
    else()
      message(FATAL_ERROR "CODE_SIGN_KEY_VAULT set to ${CODE_SIGN_KEY_VAULT} but neither LOCAL_KEY_VAULT_CERTIFICATE_THUMBPRINT nor LOCAL_KEY_VAULT_SIGNING_IDENTITY is set.")
    endif()
    configure_common_params()
    set(SIGN_PARAMS ${SIGN_PARAMS} ${store} ${certopt} ${certid}
        #/debug
        PARENT_SCOPE)
  endif()
endfunction()

function(configure_common_params)
  if (CODE_SIGN_TIMESTAMP_URL)
    set(SIGN_PARAMS ${SIGN_PARAMS}
        -fd sha256
        -td sha256 -tr ${CODE_SIGN_TIMESTAMP_URL}
        -d KTX-Software -du https://github.com/KhronosGroup/KTX-Software
        PARENT_SCOPE)
  else()
    message(FATAL_ERROR "CODE_SIGN_KEY_VAULT is set but CODE_SIGN_TIMESTAMP_URL is not set.")
  endif()
endfunction()

function(set_nsis_installer_codesign_cmd)
  # To make calls to the above macro and this order independent ...
  if(WIN32 AND CODE_SIGN_KEY_VAULT)
    if(NOT SIGN_PARAMS)
      configure_windows_sign_params()
    endif()
    if(SIGN_PARAMS)
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
