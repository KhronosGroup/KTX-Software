echo OFF
REM Add [include] of repo's .gitconfig in clone's .git/config.
REM This only needs to be run once.

REM Copyright 2016 The Khronos Group Inc.
REM SPDX-License-Identifier: Apache-2.0

REM Set colors

git config --local include.path ..\.gitconfig
echo Git config was successfully set.
