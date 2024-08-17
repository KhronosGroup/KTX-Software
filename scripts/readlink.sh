#!/bin/bash
# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# aaaaargh! macOS readlink did not support `-f` until macOS 12.3.
# CI has yet to catch up - latest is 12.2 - so this script finds
# the canonical file using just `readlink`.
libdir=$(dirname $1)
prev_target=$1

until target="$(readlink $prev_target)"; [ -z $target ]; do
  prev_target=$libdir/$target;
done

echo $prev_target
