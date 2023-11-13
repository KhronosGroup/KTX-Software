#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "KTX::ktx" for configuration "Debug"
set_property(TARGET KTX::ktx APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(KTX::ktx PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/ktx.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS KTX::ktx )
list(APPEND _IMPORT_CHECK_FILES_FOR_KTX::ktx "${_IMPORT_PREFIX}/lib/ktx.lib" )

# Import target "KTX::astcenc-avx2-static" for configuration "Debug"
set_property(TARGET KTX::astcenc-avx2-static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(KTX::astcenc-avx2-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/astcenc-avx2-static.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS KTX::astcenc-avx2-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_KTX::astcenc-avx2-static "${_IMPORT_PREFIX}/lib/astcenc-avx2-static.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
