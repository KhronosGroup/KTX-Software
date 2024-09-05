#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2020-2022 Arm Limited
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
A simple wrapper utility to run a callgrind profile over a test image, and
post-process the output into an call graph image.

Only runs on Linux and requires the following tools available on the PATH:

  * valgrind
  * gprof2dot
  * dot
"""


import argparse
import os
import re
import subprocess as sp
import sys

def postprocess_cga(lines, outfile):
    """
    Postprocess the output of callgrind_annotate.

    Args:
        lines ([str]): The output of callgrind_annotate.
        outfile (str): The output file path to write.
    """
    pattern = re.compile("^\s*([0-9,]+)\s+\([ 0-9.]+%\)\s+Source/(\S+):(\S+)\(.*\).*$")

    totalCost = 0.0
    functionTable = []
    functionMap = {}

    for line in lines:
        line = line.strip()
        match = pattern.match(line)
        if not match:
            continue

        cost = float(match.group(1).replace(",", ""))
        sourceFile = match.group(2)
        function = match.group(3)

        # Filter out library code we don't want to change
        if function.startswith("stbi__"):
            continue

        totalCost += cost

        # Accumulate the scores from functions in multiple call chains
        if function in functionMap:
            index = functionMap[function]
            functionTable[index][1] += cost
            functionTable[index][2] += cost
        # Else add new functions to the end of the table
        else:
            functionMap[function] = len(functionTable)
            functionTable.append([function, cost, cost])

    # Sort the table by accumulated cost
    functionTable.sort(key=lambda x: 101.0 - x[2])

    for function in functionTable:
        function[2] /= totalCost
        function[2] *= 100.0

    with open(outfile, "w") as fileHandle:

        totals = 0.0
        for function in functionTable:
            # Omit entries less than 1% load
            if function[2] < 1:
                break

            totals += function[2]
            fileHandle.write("%5.2f%%  %s\n" % (function[2], function[0]))

        fileHandle.write("======\n")
        fileHandle.write(f"{totals:5.2f}%\n")


def run_pass(image, noStartup, encoder, blocksize, quality):
    """
    Run Valgrind on a single binary.

    Args:
        image (str): The path of the image to compress.
        noStartup (bool): Exclude startup from reported data.
        encoder (str): The name of the encoder variant to run.
        blocksize (str): The block size to use.
        quality (str): The encoding quality to use.

    Raises:
        CalledProcessException: Any subprocess failed.
    """
    binary =  "./bin/astcenc-%s" % encoder
    args = ["valgrind", "--tool=callgrind", "--callgrind-out-file=callgrind.txt",
            binary, "-cl", image, "out.astc", blocksize, quality, "-j", "1"]

    result = sp.run(args, check=True, universal_newlines=True)

    args = ["callgrind_annotate", "callgrind.txt"]
    ret = sp.run(args, stdout=sp.PIPE, check=True, encoding="utf-8")
    lines = ret.stdout.splitlines()
    with open("perf_%s_cga.txt" % quality.replace("-", ""), "w") as handle:
        handle.write("\n".join(lines))

    postprocess_cga(lines, "perf_%s.txt" % quality.replace("-", ""))

    if noStartup:
        args = ["gprof2dot", "--format=callgrind", "--output=out.dot", "callgrind.txt",
                "-s", "-z", "compress_block(astcenc_contexti const&, image_block const&, physical_compressed_block&, compression_working_buffers&)"]
    else:
        args = ["gprof2dot", "--format=callgrind", "--output=out.dot", "callgrind.txt",
                "-s",  "-z", "main"]

    result = sp.run(args, check=True, universal_newlines=True)

    args = ["dot", "-Tpng", "out.dot", "-o", "perf_%s.png" % quality.replace("-", "")]
    result = sp.run(args, check=True, universal_newlines=True)

    os.remove("out.astc")
    os.remove("out.dot")
    os.remove("callgrind.txt")


def parse_command_line():
    """
    Parse the command line.

    Returns:
        Namespace: The parsed command line container.
    """
    parser = argparse.ArgumentParser()

    parser.add_argument("img", type=argparse.FileType("r"),
                        help="The image file to test")

    encoders = ["sse2", "sse4.1", "avx2"]
    parser.add_argument("--encoder", dest="encoder", default="avx2",
                        choices=encoders, help="select encoder variant")

    testquant = [str(x) for x in range (0, 101, 10)]
    testqual = ["-fastest", "-fast", "-medium", "-thorough", "-exhaustive"]
    qualities = testqual + testquant
    parser.add_argument("--test-quality", dest="quality", default="medium",
                        choices=qualities, help="select compression quality")

    parser.add_argument("--no-startup", dest="noStartup", default=False,
                        action="store_true", help="Exclude init")

    args = parser.parse_args()

    return args


def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """
    args = parse_command_line()
    run_pass(args.img.name, args.noStartup, args.encoder, "6x6", args.quality)
    return 0


if __name__ == "__main__":
    sys.exit(main())
