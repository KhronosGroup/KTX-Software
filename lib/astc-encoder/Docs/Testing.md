# Testing astcenc

The repository contains a small suite of tests which can be used to sanity
check source code changes to the compressor. It must be noted that this test
suite is relatively limited in scope and does not cover every feature or
bitrate of the standard.

# Required software

Running the tests requires Python 3.7 to be installed on the host machine, and
an `astcenc-avx2` release build to have been previously compiled and installed
into an directory called `astcenc` in the root of the git checkout. This
can be achieved by configuring the CMake build using the install prefix
`-DCMAKE_INSTALL_PREFIX=../` and then running a build with the `install` build
target.

# Running C++ unit tests

We support a small (but growing) number of C++ unit tests, which are written
using the `googletest` framework and integrated in the CMake "CTest" test
framework.

To build unit tests pull the `googletest` git submodule and add
`-DASTCENC_UNITTEST=ON` to the CMake command line when configuring.

To run unit tests use the CMake `ctest` utility from your build directory after
you have built the tests.

```shell
cd build
ctest --verbose
```

# Running command line tests

To run the command line tests, which aim to get coverage of the command line
options and core codec stability without testing the compression quality
itself, run the command line:

    python3 -m unittest discover -s Test -p astc_test*.py -v

# Running image tests

To run the image test suite run the following command from the root directory
of the repository:

    python3 ./Test/astc_test_image.py

This will run though a series of image compression tests, comparing the image
PSNR against a set of reference results from the last stable baseline. The test
will fail if any reduction in PSNR above a set threshold is detected. Note that
performance information is reported, but regressions will not flag a failure.

For debug purposes, all decompressed test output images and result CSV files
are stored in the `TestOutput` directory, using the same test set structure as
the `Test/Images` folder.

## Test selection

The runner supports a number of options to filter down what is run, enabling
developers to focus local testing on the parts of the code they are working on.

* `--encoder` selects which encoder to run. By default the `avx2` encoder is
  selected. Note that some out-of-tree reference encoders (older encoders, and
  some third-party encoders) are supported for comparison purposes. These will
  not work without the binaries being manually provided; they are not
  distributed here.
* `--test-set` selects which image set to run. By default the `Small` image
  test set is selected, which aims to provide basic coverage of many different
  color formats and color profiles.
* `--block-size` selects which block size to run. By default a range of
  block sizes (2D and 3D) are used.
* `--color-profile` selects which color profiles from the standard should be
  used (LDR, LDR sRGB, or HDR) to select images. By default all are selected.
* `--color-format` selects which color formats should be used (L, XY, RGB,
  RGBA) to select images. By default all are selected.

## Performance tests

To provide less noisy performance results the test suite supports compressing
each image multiple times and returning the best measured performance. To
enable this mode use the following options:

* `--repeats <M>` : Run M test compression passes which are timed.

**Note:**  The reference CSV contains performance results measured on an Intel
Core i5 9600K running at 4.3GHz, running each test 5 times.

## Updating reference data

The reference PSNR and performance scores are stored in CSVs committed to the
repository. This data is created by running the tests using the last stable
release on a standard test machine we use for performance testing builds.

It can be useful for developers to rebuild the reference results for their
local machine, in particular for measuring performance improvements. To build
new reference CSVs, download the current reference `astcenc` binary (1.7) from
GitHub for your host OS and place it in to the `./Binaries/1.7/` directory.
Once this is done, run the command:

    python3 ./Test/astc_test_image.py --encoder 1.7 --test-set all --repeats 5

... to regenerate the reference CSV files.

**WARNING:** This can take some hours to complete, and it is best done when the
test suite gets exclusive use of the machine to avoid other processing slowing
down the compression and disturbing the performance data. It is recommended to
shutdown or disable any background applications that are running.

## Valgrind memcheck

It is always worth running the Valgrind memcheck tool to validate that we have
not introduced any obvious memory errors. Build a release build with symbols
information with `-DCMAKE_BUILD_TYPE=RelWithDebInfo` and then run:

    valgrind --tool=memcheck --track-origins=yes <command>

- - -

_Copyright © 2019-2022, Arm Limited and contributors. All rights reserved._
