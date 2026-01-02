##############################################################################
# @file  FindBASH.cmake
# @brief Find BASH interpreter.
#
# Sets the CMake variables @c BASH_FOUND, @c BASH_EXECUTABLE,
# @c BASH_VERSION_STRING, @c BASH_VERSION_MAJOR, @c BASH_VERSION_MINOR, and
# @c BASH_VERSION_PATCH.
#
# @ingroup CMakeFindModules
##############################################################################

#=============================================================================
# Copyright 2011-2012 University of Pennsylvania
# Copyright 2013-2016 Andreas Schuh <andreas.schuh.84@gmail.com>
# Copyright 2013-2020 Andreas Atteneder <andreas.atteneder@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause
#
# Distributed under the OSI-approved BSD License (the "License");

# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:

# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.

# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

# ----------------------------------------------------------------------------
# find BASH executable

# First, look if GIT bash is installed
find_program (
    BASH_EXECUTABLE
    bash
PATHS
    # Additional paths for Windows
    "C:\\Program Files\\Git\\bin"
NO_SYSTEM_ENVIRONMENT_PATH
)

if(NOT BASH_EXECUTABLE)
  # Fallback search in default locations
  find_program (
      BASH_EXECUTABLE
      bash
  )
endif()

mark_as_advanced (BASH_EXECUTABLE)

# ----------------------------------------------------------------------------
# Get version of found BASH executable.
if (BASH_EXECUTABLE)
  # Set LANG to en because match looks for English "version".
  set(ENV{LANG} "en_US.UTF-8")
  execute_process (COMMAND "${BASH_EXECUTABLE}" --version OUTPUT_VARIABLE _BASH_STDOUT ERROR_VARIABLE _BASH_STDERR)
  if (_BASH_STDOUT MATCHES "version ([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set (BASH_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set (BASH_VERSION_MINOR "${CMAKE_MATCH_2}")
    set (BASH_VERSION_PATCH "${CMAKE_MATCH_3}")
    set (BASH_VERSION_STRING "${BASH_VERSION_MAJOR}.${BASH_VERSION_MINOR}.${BASH_VERSION_PATCH}")
  else ()
    message (WARNING "Failed to determine version of Bash interpreter (${BASH_EXECUTABLE})! Error:\n${_BASH_STDERR}")
  endif ()
  unset (_BASH_STDOUT)
  unset (_BASH_STDERR)
endif ()

# ----------------------------------------------------------------------------
# Handle the QUIET and REQUIRED arguments and set *_FOUND to TRUE
# if all listed variables are found or TRUE
include (FindPackageHandleStandardArgs)

find_package_handle_standard_args (
  Bash
  REQUIRED_VARS
    BASH_EXECUTABLE
)
