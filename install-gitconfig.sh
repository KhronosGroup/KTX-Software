#!/usr/bin/env bash

# Add [include] of repo's .gitconfig in clone's .git/config.

# Copyright 2016 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Set colors

GREEN='\033[1;32m'
NC='\033[0m'

git config --local include.path '../.gitconfig'
printf "${GREEN}Git config was successfully set.${NC}\n"

