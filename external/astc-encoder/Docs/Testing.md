# Testing astcenc

The repository contains a suite of tests which can be used to validate source code changes to the compressor. The test suite is split into three main test
types:

* Python tests that drive the command line functional interface tests.
* Python tests that drive command line performance and image quality tests.
* C++ unit tests for some internal codec components.

There is currently no test suite for the library API, and we try to exercise all of the functionality via the command line functional interface tests. This
gives good coverage, but is not able to reach all of the API.

In addition to the tests, we have automated Python script lint, typing, and
style checkers.

## Prerequisites

Running the tests requires Python 3.12 to be installed on the host machine.

# Running Python-based CLI functional tests

Compile the build of `astcenc` you wish to test, and install it into the project `./bin` directory. This can be achieved by configuring the CMake build using the install prefix `-DCMAKE_INSTALL_PREFIX=../` and then running a build with the `install` build target.

Run the functional tests against the appropriate encoder variant using:

```sh
python3 ./Test/astc_test_functional.py -v --encoder <encoder_variant>
```

# Running Python-based CLI performance and quality tests

Compile the build of `astcenc` you wish to test and profile, and install it into the project `./bin` directory. This can be achieved by configuring the CMake build using the install prefix `-DCMAKE_INSTALL_PREFIX=../` and then running a build with the `install` build target.

To run the image test suite run the following command from the root directory
of the repository:

```sh
python3 ./Test/astc_test_image.py
```

This will run through a series of image compression tests, comparing the image
PSNR against a set of reference results from the last stable baseline. The test
will fail if any reduction in PSNR above a set threshold is detected.

The `--test-set` parameter can be passed to select which test images you use.
The `--color-format`, `--block-size`, and `--test-quality` options allow you
to subset which compressor options are tested. The `-j` option lets you control
how many threads the compressor is allowed to use.

By default output images are discarded during testing. To store images for
debug purposes, the decompressed output images can be kept by passing
`--keep-output`. The resulting images are stored in the `TestOutput`
subdirectory, using the same test set directory structure as the `Test/Images`
folder.

## Benchmarking

All runs of this test suite will report performance data. To improve result
stability, the `--repeats` option makes the test suite process each image multiple times and return the best measured performance.

Performance regressions will not automatically cause test failure, and must be
manually reviewed.

**Note:** The upstream reference data contains performance results measured on
an Intel Core i5 9600K running at 4.3GHz, running each test 5 times.

## Updating reference data

The reference quality and performance scores are stored in CSVs committed to
the repository. This data is created by running the tests using the last stable
release on a standard test machine we use for performance testing builds.

It can be useful for developers to rebuild the reference results for their
local machine, in particular for measuring performance improvements.

To build new reference CSVs for an official release, checkout and build a
release build of the baseline release you want to compare against and place it
in to the `./Binaries/<version>/` directory, where `<version>` must be the
`major.minor` number of the release, e.g. `5.3`.

Once this is done, run the command (e.g.):

```sh
python3 ./Test/astc_test_image.py --encoder ref-5.3-avx2 ...
```

... for whichever test set you want to build a reference for. This can take
some time to complete, especially if your command line enables many test sets,
high quality levels, or uses many repeats to improve result stability.

**WARNING:** This may take some hours to complete, and it is best done when the
test suite gets exclusive use of the machine to avoid other processing slowing
down the compression and disturbing the performance data. It is recommended to
shut down or disable any background applications that are running.

# Running C++ unit tests

We support a small (but growing) number of C++ unit tests, which are written
using the googletest framework and integrated in the CMake CTest test runner.

To build unit tests pull the `googletest` git submodule and add
`-DASTCENC_UNITTEST=ON` to the CMake command line when configuring.

To run unit tests use the CMake `ctest` utility from your build directory after
you have built the tests.

```shell
cd build
ctest --verbose
```

# Using ASAN and UBSAN

Running builds compiled with ASAN (address sanitizer) and UBSAN (undefined
behavior sanitizer) is one way to check that we have not introduced memory
or undefined behavior errors. Support for both is integrated into the build
system for both GCC and Clang on Linux targets.

Build a release build with debug information using
`-DCMAKE_BUILD_TYPE=RelWithDebInfo` and specify either
`-DASTCENC_ASAN=ON` or `-DASTCENC_UBSAN=ON`.

Run test commands as normal with the generated binaries.

# Using Valgrind memcheck

Running builds using the Valgrind memcheck tool is another way to validate that
we have not introduced memory errors.

Build a release build with debug information using
`-DCMAKE_BUILD_TYPE=RelWithDebInfo`.

Run test commands through Valgrind:

    valgrind --tool=memcheck --track-origins=yes <command>

# OSS-Fuzz

ASTC-Encoder has been integrated into the Google OSS-Fuzz program, which
performs API fuzz testing on a Google-hosted CI infrastructure.

Our OSS-Fuzz test harnesses can be found in the
[/Source/Fuzzers](../Source/Fuzzers/) directory, and we have some short
[documentation](TestingOSSFuzz.md) about how to run the fuzzers locally.

The OSS-Fuzz Project is hosted on GitHub at [google/oss-fuzz][1].

The OSS-Fuzz CI is visible to maintainers at [oss-fuzz.com][2].

[1]: https://github.com/google/oss-fuzz
[2]: https://oss-fuzz.com

- - -

_Copyright © 2019-2026, Arm Limited and contributors._
