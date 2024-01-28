#! /usr/bin/env bash
# Copyright 2024 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

function usage() {
  echo "Usage: $0 [--help | --latest | --latest-pre]"
  echo ""
  echo "With no option retrieves the list of releases from the KTX-Software"
  echo "GitHub repo."
  echo ""
  echo "Options:"
  echo "  --help, -h       Print this usage message."
  echo "  --latest, -l     Retrieve information about latest release."
  echo "  --latest-pre, -p Retrieve information about latest pre-release."
  exit $1
}

ktx_repo_url=https://api.github.com/repos/KhronosGroup/KTX-Software

function get_release_list() {
  curl --silent -L $ktx_repo_url/releases
}

if [ $# -eq 0 ]; then
  get_release_list
  exit 0
elif [ $# -eq 1 ]; then
  case $1 in
    --help | -h)
      usage 0 ;;
    --latest-pre | -p)
      release_url=$(jq -r 'map(select(.prerelease)) | first | .url' <<< $(get_release_list))
      ;;
    --latest | -l)
      release_url=$ktx_repo_url/releases/latest
      ;;
    *) usage 1 ;;
  esac
else
  usage 1
fi

curl \
  --silent -L\
  -H "Accept: application/vnd.github+json" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  $release_url
