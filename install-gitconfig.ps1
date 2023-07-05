# Copyright 2023 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Add [include] of repo's .gitconfig in clone's .git/config.
# This only needs to be run once.

git config --local include.path ..\.gitconfig
echo 'Git config was successfully set.'
