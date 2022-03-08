#!/bin/bash
# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

echo "Now uploading the failed tests"
tar -cvf failed-tests.tar $TRAVIS_BUILD_DIR/tests/testimages/toktx*
# curl/transfer.sh prints the retrieval URL on completion. As the output
# is not terminated with a new-line it does not show up in Travis-CI's
# cooked output, only the raw. Workaround by saving to variable.
rurl=$(curl --upload-file failed-tests.tar https://transfer.sh/toktx-failed-tests.tar)
echo $rurl
echo "Now uploading the test log"
rurl=$(curl --upload-file $TRAVIS_BUILD_DIR/$BUILD_DIR/Testing/Temporary/LastTest.log https://transfer.sh/ktx-failed-tests.log)
echo $rurl

