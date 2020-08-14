REM Copyright 2016-2020 Mark Callow
REM SPDX-License-Identifier: Apache-2.0

REM Set up to use Git Bash for running "cygwin" actions.
REM Also used for "cygwin" rules but there paths are relativized
REM to the vcxproj location rather than the .gyp file location.
REM Note that "cygwin" actions do not relativize command
REM arguments. Relativization has to be forced by using
REM variables with names such as *_dir.

REM CD to where this file lives. "cygwin" rules will not work
REM due to this. Use 'msvs_cygwin_shell': 0.
cd %~dp0

set PATH=%PATH%;C:\Program Files\Git\bin

