#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2022 Arm Limited
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy
# of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
# -----------------------------------------------------------------------------
"""
This script is a simple test runner for sweeps on multiple compressors.
"""

import os
import subprocess as sp
import re
import sys

LOG_COMMANDS = False
LOG_PATTERN = re.compile(r"\s*Coding rate:\s*(.*)\s*MT/s")

ISPC_BIN = "./Binaries/ISPC/ispc_astc.exe"
ISPC_QUALITY = ["rgba", "rgb"]

ASTC_BIN = "./bin/astcenc-avx2"
ASTC_QUALITY = ["0", "8", "10", "20", "30", "40", "50", "60"]

TEST_BLOCK_SIZES = ["4x4", "6x6", "8x8"]

TEST_IMAGE = "./Test/Images/Kodak/LDR-RGB/ldr-rgb-kodak%02u.png"
TEST_RANGE = 24
TEST_REPEATS = 5

OUT_CIMAGE = "out.astc"
OUT_DIMAGE = "out.png"


def run(command):
    if LOG_COMMANDS:
        print(" ".join(command))

    return sp.run(command, capture_output=True, universal_newlines=True)


def run_astcenc(in_image, out_image, block_size, quality):
    args = [ASTC_BIN, "-tl", in_image, out_image, block_size, quality, "-j", "1"]
    result = run(args)
    return float(LOG_PATTERN.search(result.stdout).group(1))


def run_ispc(in_image, out_image, block_size, quality):
    args = [ISPC_BIN, in_image, out_image, block_size, quality]
    result = run(args)
    return float(LOG_PATTERN.search(result.stdout).group(1))


def decompress(in_image, out_image):
    args = [ASTC_BIN, "-dl", in_image, out_image]
    result = run(args)
    os.remove(in_image)


def compare(in_image, out_image):
    args = ["compare", "-metric", "PSNR", in_image, out_image, "diff.png"]
    result = run(args)
    os.remove("diff.png")
    os.remove(out_image)
    return float(result.stderr)


def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """
    # ISPC Tests
    for block_size in TEST_BLOCK_SIZES:
        for quality in ISPC_QUALITY:
            print(f"ISPC {quality} {block_size}")
            print(f"ISPC {quality} {block_size}", file=sys.stderr)
            for index in range(1, TEST_RANGE + 1):
                result_rate = 0.0
                for repeat in range(0, TEST_REPEATS):
                    image = TEST_IMAGE % index
                    result_rate += run_ispc(image, OUT_CIMAGE, block_size, quality)
                    decompress(OUT_CIMAGE, OUT_DIMAGE)
                    result_error = compare(image, OUT_DIMAGE)
                result_rate /= TEST_REPEATS

                print("%s,Kodak%02u,%0.4f,%0.4f" % (block_size, index, result_rate, result_error))

    # ASTCENC Tests
    for block_size in TEST_BLOCK_SIZES:
        for quality in ASTC_QUALITY:
            print(f"ASTC {quality} {block_size}")
            print(f"ASTC {quality} {block_size}", file=sys.stderr)
            for index in range(1, TEST_RANGE + 1):
                result_rate = 0.0
                for repeat in range(0, TEST_REPEATS):
                    image = TEST_IMAGE % index
                    result_rate += run_astcenc(image, OUT_DIMAGE, block_size, quality)
                    result_error = compare(image, OUT_DIMAGE)
                result_rate /= TEST_REPEATS

                print("%s,Kodak%02u,%0.4f,%0.4f" % (block_size, index, result_rate, result_error))

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except sp.CalledProcessError as ex:
        print(ex.stdout)
        print(ex.stderr)
