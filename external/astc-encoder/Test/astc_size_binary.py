#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2019-2026 Arm Limited
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
'''
This script is a wrapper around the Linux and macOS 'size' utility to extract
information about binary section sizes, and optionally compare the section
sizes of two binaries.

Section sizes are reported for code (.text), read-only data (.rodata), and zero
initialized data (.bss) sections. All other sections are ignored.

A typical report comparing the size of two binaries looks like this:

               Code   RO Data   ZI Data
      Ref    411298    374560    128576
      New    560530     89552     31744
    Abs D    149232   -285008    -96832
    Rel D    36.28%   -76.09%   -75.31%
'''

import argparse
import platform
import shutil
import subprocess as sp
import sys


def run_size_linux(binary: str) -> tuple[int, int, int]:
    '''
    Run size on a single binary.

    Args:
        binary: The path of the binary file to analyze.

    Return:
        A triplet of code size, RO data size, ZI data size.

    Raise:
        CalledProcessException: The size subprocess failed.
    '''
    args = ['size', '--format=sysv', binary]
    result = sp.run(args, stdout=sp.PIPE, stderr=sp.PIPE,
                    check=True, text=True)

    data = {}
    patterns = {'Code': '.text', 'RO': '.rodata', 'ZI': '.bss'}

    for line in result.stdout.splitlines():
        for key, value in patterns.items():
            if line.startswith(value):
                size = int(line.split()[1])
                data[key] = size

    return (data['Code'], data['RO'], data['ZI'])


def run_size_macos(binary: str) -> tuple[int, int, int]:
    '''
    Run size on a single binary.

    Args:
        binary: The path of the binary file to analyze.

    Return:
        A triplet of code size, RO data size, ZI data size.

    Raise:
        CalledProcessException: The size subprocess failed.
    '''
    args = ['size', '-m', binary]
    result = sp.run(args, stdout=sp.PIPE, stderr=sp.PIPE,
                    check=True, text=True)

    size_code = 0
    size_ro = 0
    size_zi = 0

    current_segment = None

    for line in result.stdout.splitlines():
        line = line.strip()

        if line.startswith('Segment'):
            parts = line.split()
            assert len(parts) >= 3, parts

            current_segment = parts[1]
            size = int(parts[2])

            if current_segment == '__TEXT:':
                size_code += size

            if current_segment == '__DATA_CONST:':
                size_ro += size

            if current_segment == '__DATA:':
                size_zi += size

        if line.startswith('Section'):
            parts = line.split()
            assert len(parts) >= 3, parts

            section = parts[1]
            size = int(parts[2])

            if current_segment == '__TEXT:' and section == '__const:':
                size_code -= size
                size_ro += size

    return (size_code, size_ro, size_zi)


def parse_command_line() -> argparse.Namespace:
    '''
    Parse the command line.

    Return:
        The parsed command line.
    '''
    parser = argparse.ArgumentParser()

    parser.add_argument('bin', type=argparse.FileType('r'),
                        help='the binary to size')

    parser.add_argument('ref_bin', nargs='?', type=argparse.FileType('r'),
                        help='the reference binary to compare against')

    return parser.parse_args()


def main() -> int:
    '''
    The main function.

    Return:
        The process return code.
    '''
    args = parse_command_line()

    # Preflight - check that size exists
    path = shutil.which('size')
    if not path:
        print('ERROR: The size utility is not installed')
        return 1

    # Pick an appropriate implementation based on host OS
    run_size = run_size_linux
    if platform.system() == 'Darwin':
        run_size = run_size_macos

    # Collect the data
    try:
        ref_dat = None
        if args.ref_bin:
            ref_dat = run_size(args.ref_bin.name)

        new_dat = run_size(args.bin.name)

    except sp.CalledProcessError as ex:
        print('ERROR: The size utility failed')
        print(ex.stderr.strip())
        return 1

    # Common header
    print('              Code        RO        ZI')

    # One binary, no reference comparison
    if not ref_dat:
        print(f'     New{new_dat[0]:>10}{new_dat[1]:>10}{new_dat[2]:>10}')

    # Two binaries with reference comparison
    else:
        print(f'     Ref{ref_dat[0]:>10}{ref_dat[1]:>10}{ref_dat[2]:>10}')
        print(f'     New{new_dat[0]:>10}{new_dat[1]:>10}{new_dat[2]:>10}')

        df_a = [y - x for (x, y) in zip(ref_dat, new_dat)]
        df_r = [((y - x) / x) * 100.0 for (x, y) in zip(ref_dat, new_dat)]

        print(f'Diff abs{df_a[0]:>+10}{df_a[1]:>+10}{df_a[2]:>+10}')
        print(f'Diff rel{df_r[0]:>+10.2f}%{df_r[1]:>+9.2f}%{df_r[2]:>+9.2f}%')

    return 0


if __name__ == '__main__':
    sys.exit(main())
