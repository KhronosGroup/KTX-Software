#!/bin/bash
# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

if [ $# -ge 1 ]; then
  repo_root=$1
else
  repo_root=$TRAVIS_BUILD_DIR
fi
if [ $# -eq 2 ]; then
  build_dir=$2
else
  build_dir=$TRAVIS_BUILD_DIR/$BUILD_DIR
fi

echo "Now checking for and uploading any failed tests"
image_list="$repo_root/tests/testimages/ktx2ktx2* $repo_root/tests/testimages/ktxsc*, $repo_root/tests/testimages/toktx*"
if tar -cvf failed-tests.tar $image_list
then
  # curl/transfer.sh prints the retrieval URL on completion. As the output
  # is not terminated with a new-line it does not show up in Travis-CI's
  # cooked output, only the raw. Workaround by saving to variable.
  rurl=$(curl --upload-file failed-tests.tar https://transfer.sh/toktx-failed-tests.tar)
  echo $rurl
fi
echo "Now uploading the test log"
rurl=$(curl --upload-file $build_dir/Testing/Temporary/LastTest.log https://transfer.sh/ktx-failed-tests.log)
echo $rurl
