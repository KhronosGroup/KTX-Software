# Copyright 2022 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

if ($args[0]) { $repo_root = $args[0] } else { $repo_root = "." }
if ($args[1]) { $build_dir = $args[1] } else { $build_dir = "build" }

# In Github Actions, this is called only when there is a failed test step.
# In Appveyor we use the Phase environment variable.
echo "Phase = $env:Phase"
if ($env:GITHUB_ACTIONS -or $env:Phase) {
  pushd $repo_root
  echo "Now uploading the failed tests"
  $image_list = "tests/testimages/ktx2ktx2*", "tests/testimages/ktxsc*", "tests/testimages/toktx*"
  ls $image_list
  echo "Current directory is"
  pwd
  if (tar -cvf failed-images.tar $image_list) {
    # N.B. In PS, "curl" is an alias for Invoke-WebRequest. Uploading a
    # file via that looks like a p.i.t.a so use the real curl command.
    curl.exe --upload-file failed-images.tar https://transfer.sh/ktx-failed-images.tar
  }
  # Even if there are no failed images and tar exits with false it creates
  # the output file.
  rm failed-images.tar

  echo "`r`nNow uploading the test log"
  curl.exe --upload-file $build_dir/Testing/Temporary/LastTest.log https://transfer.sh/ktx-last-test.log
  popd
}
