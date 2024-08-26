#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2021-2022 Arm Limited
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
A benchmarking sweep helper, which can generate a performance-vs-quality sweep
for a single input images. Like other test functionality, this uses structured
image directory layouts for determining image settings to pass to the codec.
"""

import argparse
import os
import platform
import sys

import testlib.encoder as te
import testlib.image as ti


def parse_command_line():
    """
    Parse the command line.

    Returns:
        Namespace: The parsed command line container.
    """
    parser = argparse.ArgumentParser()

    # All reference encoders
    parser.add_argument("--step", dest="step", default="10", type=int, help="step size")
    parser.add_argument("--repeats", dest="repeats", type=int, default=1, help="repeats")

    parser.add_argument(dest="image", default=None,
                        help="select the test image to run")

    args = parser.parse_args()
    return args


def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """
    # Parse command lines
    args = parse_command_line()

    blockSizes = ["4x4", "5x5", "6x6", "8x8", "10x10"]
    repeats = max(args.repeats, 1)
    step = max(args.step, 1)

    image = ti.TestImage(args.image)
    codec = te.Encoder2x("avx2")

    print("Block Size, Quality, PSNR (dB), Coding Time (s), Coding Rate (MT/s)")

    for blockSize in blockSizes:
        for quality in range(0, 101, args.step):
            localRepeats = repeats
            if quality < 20:
                localRepeats = localRepeats * 2
            if quality < 40:
                localRepeats = localRepeats * 2

            results = codec.run_test(image, blockSize, f"{quality}", localRepeats, False)
            psnr = results[0]
            codingTime = results[2]
            mts = results[3]

            print(f"{blockSize}, {quality}, {psnr}, {codingTime}, {mts}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
