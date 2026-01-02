#! /usr/bin/env bash
# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Build WebAssembly with Emscripten in Docker.

# Exit if any command fails.
set -e

docker_running=0
rm_container=0

function atexit () {
  if [ $docker_running -eq 0 ]; then
    docker stop emscripten > /dev/null
    # By default keep the container as it now has a populated emscripten cache.
    if [ $rm_container -eq 1 ]; then
      docker rm emscripten > /dev/null
    fi
  fi
}

function usage() {
  echo "Usage: $0 [Option] [PARAMETER=value] [target...]"
  echo ""
  echo "Build KTX-Software using Emscripten docker package."
  echo "Defaults to building everything (Release config) if no targets specified."
  echo "Options:"
  echo "  --help, -h      Print this usage message."
  echo "  --remove-container, -r    Remove docker container when finished."
  echo "  --verbose ,-v   Cause the underlying make to run in verbose mode."
  exit $1
}

# cd repo root so script will work whereever the current directory
path_to_repo_root=..
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/$path_to_repo_root"

for arg in $@; do
  case $arg in
    --help | -h)
      usage 0
      ;;
    --remove-container | -r)
      rm_container=1
      shift
      ;;
    --verbose | -v)
      verbose_make="-- VERBOSE=1"
      shift
      ;;
    *\=*)
      # Set parameter from command-line arguments
      eval $arg
      shift ;;
    *)
      targets="$targets --target $arg"
      shift ;;
  esac
done

# Set defaults
CONFIGURATION=${CONFIGURATION:-Release}
FEATURE_DOC=${FEATURE_DOC:-OFF}
FEATURE_JNI=${FEATURE_JNI:-OFF}
FEATURE_LOADTESTS=${FEATURE_LOADTESTS:-OpenGL}
FEATURE_PY=${FEATURE_PY:-OFF}
FEATURE_TESTS=${FEATURE_TESTS:-OFF}
FEATURE_TOOLS=${FEATURE_TOOLS:-OFF}
PACKAGE=${PACKAGE:-NO}
SUPPORT_SSE=OFF
SUPPORT_OPENCL=${SUPPORT_OPENCL:-OFF}
WERROR=${WERROR:-OFF}

BUILD_DIR=${BUILD_DIR:-build/web-$CONFIGURATION}

trap atexit EXIT SIGINT

# Check emscripten container status.
if ! container_status="$(docker container inspect -f '{{.State.Status}}' emscripten 2> /dev/null)"; then
  # `inspect` failed therefore no docker container. Create one.

  #   In the event that /src in docker ends up having the same owner as
  # the repo (cwd) on the docker host, which is presumably the user running
  # this script, it is necessary to set the uid/gid used to run the
  # commands in docker. This is because the recent fix for
  # CVE-2022-24765 causes Git to error when the repo owner differs from
  # the user running the command and the default docker user running the
  # commands is root. For details see
  # https://github.blog/2022-04-12-git-security-vulnerability-announced/
  #   When I run docker locally on macOS /src is owned by root so we don't
  # trip over the CVE fix. However in Linux CI runners, on both Travis and
  # GitHub, /src ends up owned by the same uid as the repo on the host.
  # Since .github/workflows/web.yml starts docker before calling this
  # script, the correct uid is set there. This is retained as an example
  # in case this is related to Linux rather than GHA/Travis or some other
  # system exhibits the same behavior.
  if [ -n "$TRAVIS" ]; then
    ugids="--user $(id -u):$(id -g)"
  fi
  echo "Starting Enscripten Docker container"
  docker run -dit --name emscripten $ugids -v $(pwd):/src emscripten/emsdk bash
else
  if [ "$container_status" = "running" ]; then
    # CI has already started it.
    echo "Emscripten Docker container running"
    dockerrunning=1
  elif [ "$container_status" = "exited" ]; then
    # It wasn't removed after previous run. Resume it for use of Emscripten cache.
    echo "Resuming Emscripten Docker container"
    docker start emscripten
  else
    echo "Emscripten container is in unsupported state."
    exit 1
  fi
fi

echo "Software versions"
echo '*****************'
docker exec emscripten sh -c "emcc -v; echo '********'"
docker exec emscripten sh -c "cmake --version; echo '********'"
docker exec emscripten sh -c "git --version; echo '********'"

mkdir -p $BUILD_DIR

# emcmake uses the "Unix Makefiles" generator on Linux which does not
# support multiple configurations.
echo "Configure and Build KTX-Software (Web $CONFIGURATION)"

# Uncomment for debugging some generator expressions.
#targets="--target debug_isgnufe1 --target debug_gnufe_ffpcontract"

# Since 4.0.9 SDL2 has to be installed in order for its CMake config
# file to be found.
if [ -n "$FEATURE_LOADTESTS" ]; then
  docker exec emscripten sh -c "embuilder build sdl3"
fi

docker exec emscripten sh -c "emcmake cmake -B$BUILD_DIR . \
    -D CMAKE_BUILD_TYPE=$CONFIGURATION \
    -D KTX_FEATURE_DOC=$FEATURE_DOC \
    -D KTX_FEATURE_JNI=$FEATURE_JNI \
    -D KTX_FEATURE_LOADTEST_APPS=$FEATURE_LOADTESTS \
    -D KTX_FEATURE_TESTS=$FEATURE_TESTS \
    -D KTX_FEATURE_TOOLS=$FEATURE_TOOLS \
    -D KTX_WERROR=$WERROR \
  && cmake --build $BUILD_DIR $targets $verbose_make"

if [ "$PACKAGE" = "YES" ]; then
  echo "Pack KTX-Software (Web $CONFIGURATION)"
  # Call cmake rather than cpack so we don't need knowledge of the working
  # directory inside docker.
  docker exec emscripten sh -c "cmake --build $BUILD_DIR --target package"
fi

