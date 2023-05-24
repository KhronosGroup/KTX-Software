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
The ``astc_quality_test`` utility provides a tool to sweep quality settings.
"""

import numpy as np
import re
import subprocess as sp
import sys


def get_psnr_pattern():
    return r"\s*PSNR \(LDR-RGB\):\s*([0-9.]*) dB"


def get_coding_rate_pattern():
    return r"\s*Coding rate:\s*([0-9.]*) MT/s"


def parse_output(output):
    # Regex pattern for image quality
    patternPSNR = re.compile(get_psnr_pattern())
    patternCRate = re.compile(get_coding_rate_pattern())

    # Extract results from the log
    runPSNR = None
    runCRate = None

    for line in output:
        match = patternPSNR.match(line)
        if match:
            runPSNR = float(match.group(1))

        match = patternCRate.match(line)
        if match:
            runCRate = float(match.group(1))

    assert runPSNR is not None, "No coding PSNR found"
    assert runCRate is not None, "No coding rate found"
    return (runPSNR, runCRate)

def execute(command):
    """
    Run a subprocess with the specified command.

    Args:
        command (list(str)): The list of command line arguments.

    Returns:
        list(str): The output log (stdout) split into lines.
    """
    try:
        result = sp.run(command, stdout=sp.PIPE, stderr=sp.PIPE,
                        check=True, universal_newlines=True)
    except (OSError, sp.CalledProcessError):
        print("ERROR: Test run failed")
        print("  + %s" % " ".join(command))
        qcommand = ["\"%s\"" % x for x in command]
        print("  + %s" % ", ".join(qcommand))
        sys.exit(1)

    return result.stdout.splitlines()

def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """
    for block in ("4x4", "5x5", "6x6", "8x8", "10x10"):

        for quality in range (0, 101, 2):

            resultsQ = []
            resultsS = []

            if (quality < 40):
                repeats = 20
            elif (quality < 75):
                repeats = 10
            else:
                repeats = 5

            for _ in range(0, repeats):
                command = [
                    "./bin/astcenc-avx2",
                    "-tl",
                    "./Test/Images/Kodak/LDR-RGB/ldr-rgb-kodak23.png",
                    "/dev/null",
                    block,
                    "%s" % quality,
                    "-silent"
                ]

                stdout = execute(command)
                psnr, mts = parse_output(stdout)
                resultsQ.append(psnr)
                resultsS.append(mts)

            print("%s, %u, %0.3f, %0.3f" % (block, quality, np.mean(resultsS), np.mean(resultsQ)))


    return 0


if __name__ == "__main__":
    sys.exit(main())
