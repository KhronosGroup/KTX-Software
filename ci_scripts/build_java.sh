#!/bin/bash
# Copyright 2021 Shukant Pal
# SPDX-License-Identifier: Apache-2.0

echo "Build Java bindings for KTX-Software"
echo "LIBKTX_BINARY_DIR " $LIBKTX_BINARY_DIR

build_libktx_java_dir=interface/java_binding

pushd $build_libktx_java_dir
LIBKTX_BINARY_DIR=$LIBKTX_BINARY_DIR mvn package
popd
