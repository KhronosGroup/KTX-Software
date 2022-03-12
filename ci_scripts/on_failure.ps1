# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

echo "Phase = $env:Phase"
if (! $env:Phase) {
  echo "Exiting on_failure script."
  exit
}

echo "Now uploading the failed tests"
ls -Name $env:APPVEYOR_BUILD_FOLDER/tests/testimages/toktx*
tar -cvf failed-tests.tar $env:APPVEYOR_BUILD_FOLDER/tests/testimages/toktx*
curl --upload-file failed-tests.tar https://transfer.sh/toktx-failed-tests.tar
echo "Now uploading the test log"
curl --upload-file $env:APPVEYOR_BUILD_FOLDER/$env:BUILD_DIR/Testing/Temporary/LastTest.log https://transfer.sh/ktx-last-test.log

