#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2019-2020 Arm Limited
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
The ``astc_size_binary`` utility provides a wrapper around the Linux ``size``
utility to view binary section sizes, and optionally compare the section sizes
of two binaries. Section sizes are given for code (``.text``), read-only data
(``.rodata``), and zero initialized data (``.bss``). All other sections are
ignored.

A typical report comparing the size of a new binary against a reference looks
like this:

.. code-block::

              Code   RO Data   ZI Data
     Ref    411298    374560    128576
     New    560530     89552     31744
   Abs D    149232   -285008    -96832
   Rel D    36.28%   -76.09%   -75.31%
"""


import argparse
import platform
import shutil
import subprocess as sp
import sys


def run_size_linux(binary):
    """
    Run size on a single binary.

    Args:
        binary (str): The path of the binary file to process.

    Returns:
        tuple(int, int, int): A triplet of code size, read-only data size, and
        zero-init data size, all in bytes.

    Raises:
        CalledProcessException: The ``size`` subprocess failed for any reason.
    """
    args = ["size", "--format=sysv", binary]
    result = sp.run(args, stdout=sp.PIPE, stderr=sp.PIPE,
                    check=True, universal_newlines=True)

    data = {}
    patterns = {"Code": ".text", "RO": ".rodata", "ZI": ".bss"}

    lines = result.stdout.splitlines()
    for line in lines:
        for key, value in patterns.items():
            if line.startswith(value):
                size = float(line.split()[1])
                data[key] = size

    return (data["Code"], data["RO"], data["ZI"])


def run_size_macos(binary):
    """
    Run size on a single binary.

    Args:
        binary (str): The path of the binary file to process.

    Returns:
        tuple(int, int, int): A triplet of code size, read-only data size, and
        zero-init data size, all in bytes.

    Raises:
        CalledProcessException: The ``size`` subprocess failed for any reason.
    """
    args = ["size", "-m", binary]
    result = sp.run(args, stdout=sp.PIPE, stderr=sp.PIPE,
                    check=True, universal_newlines=True)

    code = 0
    dataRO = 0
    dataZI = 0

    currentSegment = None

    lines = result.stdout.splitlines()
    for line in lines:
        line = line.strip()

        if line.startswith("Segment"):
            parts = line.split()
            assert len(parts) >= 3, parts

            currentSegment = parts[1]
            size = int(parts[2])

            if currentSegment == "__TEXT:":
                code += size

            if currentSegment == "__DATA_CONST:":
                dataRO += size

            if currentSegment == "__DATA:":
                dataZI += size

        if line.startswith("Section"):
            parts = line.split()
            assert len(parts) >= 3, parts

            section = parts[1]
            size = int(parts[2])

            if currentSegment == "__TEXT:" and section == "__const:":
                code -= size
                dataRO += size

    return (code, dataRO, dataZI)


def parse_command_line():
    """
    Parse the command line.

    Returns:
        Namespace: The parsed command line container.
    """
    parser = argparse.ArgumentParser()

    parser.add_argument("bin", type=argparse.FileType("r"),
                        help="The new binary to size")

    parser.add_argument("ref", nargs="?", type=argparse.FileType("r"),
                        help="The reference binary to compare against")

    return parser.parse_args()


def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """
    args = parse_command_line()

    # Preflight - check that size exists. Note that size might still fail at
    # runtime later, e.g. if the binary is not of the correct format
    path = shutil.which("size")
    if not path:
        print("ERROR: The 'size' utility is not installed on the PATH")
        return 1

    if platform.system() == "Darwin":
        run_size = run_size_macos
    else:
        run_size = run_size_linux

    # Collect the data
    try:
        newSize = run_size(args.bin.name)
        if args.ref:
            refSize = run_size(args.ref.name)
    except sp.CalledProcessError as ex:
        print("ERROR: The 'size' utility failed")
        print("       %s" % ex.stderr.strip())
        return 1

    # Print the basic table of absolute values
    print("%8s  % 8s  % 8s  % 8s" % ("", "Code", "RO Data", "ZI Data"))
    if args.ref:
        print("%8s  % 8u  % 8u  % 8u" % ("Ref", *refSize))
    print("%8s  % 8u  % 8u  % 8u" % ("New", *newSize))

    # Print the difference if we have a reference
    if args.ref:
        diffAbs = []
        diffRel = []
        for refVal, newVal in zip(refSize, newSize):
            diff = newVal - refVal
            diffAbs.append(diff)
            diffRel.append((diff / refVal) * 100.0)

        dat = ("Abs D", diffAbs[0], diffAbs[1], diffAbs[2])
        print("%8s  % 8u  % 8u  % 8u" % dat)
        dat = ("Rel D", diffRel[0], diffRel[1], diffRel[2])
        print("%8s  % 7.2f%%  % 7.2f%%  % 7.2f%%" % dat)

    return 0


if __name__ == "__main__":
    sys.exit(main())
