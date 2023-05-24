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
The ``astc_dump_binary`` utility provides a wrapper around the ``objdump``
utility to extract disassembly of specific functions. Currently only matches
the root name, for sake of command line sanity, so all overloads get dumped.

Using __attribute__((noinline)) can be useful during profiling to stop any
functions of interest getting inlined once they get too small ...
"""


import argparse
import re
import shutil
import subprocess as sp
import sys


def run_objdump(binary, symbol):
    """
    Run objdump on a single binary and extract the range for a given symbol.

    Output is printed to stdout.

    Args:
        binary (str): The path of the binary file to process.
        symbol (str): The symbol to match.

    Raises:
        CalledProcessException: The ``objdump`` subprocess failed for any reason.
    """
    args = [
        "objdump", "-C",
        "-M", "intel",
        "--no-show-raw",
        "-d", "-S",
        binary
    ]

    result = sp.run(args, stdout=sp.PIPE, stderr=sp.PIPE,
                    check=True, universal_newlines=True)

    funcPattern = re.compile(r"^[0-9a-f]{16} <(.*?)\(.*\)>:$")

    funcLines = []
    funcActive = False
    lines = result.stdout.splitlines()

    for line in lines:
        match = funcPattern.match(line)
        if match:
            funcName = match.group(1)
            if funcName == symbol:
                funcActive = True
            else:
                funcActive = False

        if funcActive:
            funcLines.append(line)

    print("\n".join(funcLines))

def parse_command_line():
    """
    Parse the command line.

    Returns:
        Namespace: The parsed command line container.
    """
    parser = argparse.ArgumentParser()

    parser.add_argument("binary", type=argparse.FileType("r"),
                        help="The new binary to dump")

    parser.add_argument("symbol", type=str,
                        help="The function name to dump")

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
    path = shutil.which("objdump")
    if not path:
        print("ERROR: The 'objdump' utility is not installed on the PATH")
        return 1

    # Collect the data
    try:
        run_objdump(args.binary.name, args.symbol)
    except sp.CalledProcessError as ex:
        print("ERROR: The 'objdump' utility failed")
        print("       %s" % ex.stderr.strip())
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
