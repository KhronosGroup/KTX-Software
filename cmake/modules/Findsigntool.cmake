#[============================================================================
# Copyright 2022, Khronos Group, Inc.
# SPDX-License-Identifier: Apache-2.0
#============================================================================]

#  Functions to convert unix-style paths into paths useable by cmake on windows.
#[=======================================================================[.rst:
Findsigntool
-------

Finds the signtool executable used for codesigning on Windows.

Note that signtool does not offer a way to make it print its version
so version selection and reporting is not possible.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``signtool_FOUND``
  True if the system has the signtool executable.
``signtool_EXECUTABLE``
  The signtool command executable.

#]=======================================================================]

if (WIN32 AND CMAKE_HOST_SYSTEM_NAME MATCHES "CYGWIN.*")
  find_program(CYGPATH
      NAMES cygpath
      HINTS [HKEY_LOCAL_MACHINE\\Software\\Cygwin\\setup;rootdir]/bin
      PATHS C:/cygwin64/bin
            C:/cygwin/bin
  )
endif ()

function(convert_cygwin_path _pathvar)
  if (WIN32 AND CYGPATH)
    execute_process(
        COMMAND         "${CYGPATH}" -m "${${_pathvar}}"
        OUTPUT_VARIABLE ${_pathvar}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${_pathvar} "${${_pathvar}}" PARENT_SCOPE)
  endif ()
endfunction()

function(convert_windows_path _pathvar)
  if (CYGPATH)
    execute_process(
        COMMAND         "${CYGPATH}" "${${_pathvar}}"
        OUTPUT_VARIABLE ${_pathvar}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${_pathvar} "${${_pathvar}}" PARENT_SCOPE)
  endif ()
endfunction()

# Make a list of Windows Kit versions with newer versions first.
#
# _root     string          KitsRoot for the Windows version whose kits to find.
# _versions variable name   Variable in which to return the list of versions.
#
function(find_kits _winver _kit_versions)
  set(${_kit_versions})
  set(_kit_root "KitsRoot${_winver}")
  set(regkey "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots")
  set(regval ${_kit_root})
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    # Note: must be a cache operation in order to read from the registry.
    get_filename_component(_kits_path "[${regkey};${regval}]"
        ABSOLUTE CACHE
    )
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "CYGWIN.*")
    # On Cygwin, CMake's built-in registry query won't work.
    # Use Cygwin utility "regtool" instead.
    execute_process(COMMAND regtool get "\\${regkey}\\${regval}"
      OUTPUT_VARIABLE _kits_path}
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (_kits_path)
      convert_windows_path(_kits_path)
    endif ()
  endif()
  if (_kits_path)
      file(GLOB ${_kit_versions} "${_kits_path}/bin/${_winver}.*")
      # Reverse list, so newer versions (higher-numbered) appear first.
      list(REVERSE ${_kit_versions})
  endif ()
  unset(_kits_path CACHE)
  set(${_kit_versions} ${${_kit_versions}} PARENT_SCOPE)
endfunction()

if (WIN32 AND NOT signtool_EXECUTABLE)
  if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
    set(arch "x64")
  else()
    set(arch ${CMAKE_HOST_SYSTEM_PROCESSOR})
  endif()

  # Look for latest signtool
  foreach(winver 11 10)
    find_kits(${winver} kit_versions)
    if (kit_versions)
      find_program(signtool_EXECUTABLE
          NAMES           signtool
          PATHS           ${kit_versions}
          PATH_SUFFIXES   ${arch}
                          bin/${arch}
                          bin
          NO_DEFAULT_PATH
      )
      if (signtool_EXECUTABLE)
        break()
      endif()
    endif()
  endforeach()

  if (signtool_EXECUTABLE)
    mark_as_advanced (signtool_EXECUTABLE)
  endif ()

  # handle the QUIETLY and REQUIRED arguments and set *_FOUND to TRUE
  # if all listed variables are found or TRUE
  include (FindPackageHandleStandardArgs)

  find_package_handle_standard_args (
    signtool
    REQUIRED_VARS
      signtool_EXECUTABLE
    FAIL_MESSAGE
      "Could NOT find signtool. Will be unable to sign Windows binaries."
  )
endif()
