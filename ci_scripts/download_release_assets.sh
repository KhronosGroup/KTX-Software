#! /usr/bin/env bash
# Copyright 2024 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

function usage() {
  echo "Usage: $0 [--help] [--pre-release] [--filter <pattern>] [--output-dir <directory>]"
  echo ""
  echo "Download the assets of the latest KTX-Software release from the"
  echo "GitHub repo."
  echo ""
  echo "Options:"
  echo "  --filter, -f <pattern>"
  echo "             Only download assets whose name matches <pattern> where"
  echo "             pattern is a regular expression."
  echo "  --help, -h Print this usage message."
  echo "  --output-dir <directory>"
  echo "             Save the downloaded files in <directory>. Defaults to"
  echo "             the current directory."
  echo "  --pre-release, -p"
  echo "             Retrieve assets from latest pre-release."
  exit $1
}

# list_releases.sh should be colocated with this script
# and the directory may not be in $PATH.
get_release=$(dirname $0)/list_releases.sh

target="--latest"
output_dir="."

while [ $# -ne 0 ]; do
  case $1 in
    --filter | -f)
      if [ $# -lt 1 ]; then
        usage 1
      else
        pattern=$2
        shift 2
      fi
      ;;
    --help | -h)
      usage 0;;
    --output-dir | -o)
      if [ $# -lt 1 ]; then
        usage 1
      else
        output_dir=$2
        shift 2
      fi
      ;;
    --pre-release | -p)
      target="--latest-pre"
      shift;;
    *) usage 1;;
  esac
done

# Retrieve target release metadata, extract and filter assets and extract
# their browser_download_urls using jq (jquery).
jq ".assets | map(select(.name | test(\"$pattern\"))) | map(.browser_download_url)" <<< $($get_release $target) | jq -c '.[]' | xargs -L 1 curl -O -J -L --create-dirs --output-dir $output_dir
