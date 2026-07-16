#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2020-2026 Arm Limited
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
This script is a wrapper around Valgrind that will capture a callgrind profile
for a single test image, and then use gprof2dot and dot to produce an annotated
call graph image.

This script requires the following tools available on the PATH:

  * valgrind
  * dot (part of graphviz)

... and the following Python modules:

  * gprof2dot
'''

import argparse
from collections import defaultdict
import os
import re
import subprocess as sp
import sys


def rank_hot_functions(lines: list[str]) -> str:
    '''
    Generate a summary of hot functions in a callgrind_annotate log.

    Args:
        lines: The output of callgrind_annotate.

    Return:
        The report ranking the hot functions.
    '''
    # Parse pattern for useful callgrind lines
    parse_pattern = re.compile(
        r'^\s*([0-9,]+)\s+\([ 0-9.]+%\)\s+Source/(\S+):(\S+)\(.*\).*$'
    )

    function_cost: dict[str, float] = defaultdict(float)

    for line in lines:
        line = line.strip()
        match = parse_pattern.match(line)
        if not match:
            continue

        cost = float(match.group(1).replace(',', ''))
        source_file = match.group(2)
        function = match.group(3)

        # Filter out third-party library code
        if source_file.startswith('ThirdParty'):
            continue

        # Accumulate the scores from functions in multiple call chains
        function_cost[function] += cost

    # Sum the total cost of the profile
    total_cost = sum(function_cost.values())

    # Sort functions into most expensive first
    function_table = list(function_cost.items())
    function_table.sort(key=lambda x: x[1], reverse=True)

    report: list[str] = []
    total_reported = 0.0
    for func, func_cost in function_table:
        func_percent = (func_cost / total_cost) * 100.0

        # Stop emitting entries when we get to less than 1% contribution
        if func_percent < 1.0:
            break

        total_reported += func_percent
        report.append(f'{func_percent:5.2f}%  {func}')

    report.append('======')
    report.append(f'{total_reported:5.2f}%')

    return '\n'.join(report)


def run_pass(image: str, encoder: str,
             block_size: str, quality: str, show_cli: bool) -> None:
    '''
    Run Valgrind on a single binary.

    Args:
        image: The path of the image to compress.
        encoder: The encoder variant to use.
        block_size: The block size to use.
        quality: The compression quality to use.
        show_cli: Show command line wrapper tool cost in the profile.

    Raise:
        CalledProcessException: Any subprocess failed.
    '''
    binary = f'./bin/astcenc-{encoder}'
    quality_log = quality.replace('-', '')

    # Run Valgrind
    args = [
        'valgrind',
        '--tool=callgrind',
        '--callgrind-out-file=callgrind.txt',
        binary, '-ch', image, '/dev/null', block_size, quality, '-j', '1'
    ]

    sp.run(args, check=True)

    # Run callgrind analysis
    args = [
        'callgrind_annotate',
        'callgrind.txt'
    ]

    ret = sp.run(args, stdout=sp.PIPE, check=True, encoding='utf-8')
    lines = ret.stdout.splitlines()

    # Write raw annotated callgrind log
    report = '\n'.join(lines)
    with open(f'perf_{quality_log}_cga.txt', 'w', encoding='utf=8') as handle:
        handle.write(report)

    # Write our text summary of the top functions
    report = rank_hot_functions(lines)
    with open(f'perf_{quality_log}.txt', 'w', encoding='utf=8') as handle:
        handle.write(report)

    # Write the visual call graph
    entry_func = 'main'
    if not show_cli:
        entry_func = 'compress_block(' \
            'astcenc_contexti const&, ' \
            'image_block const&, ' \
            'unsigned char*, ' \
            'compression_working_buffers&)'

    args = [
        'gprof2dot',
        '--format=callgrind',
        '--output=out.dot',
        'callgrind.txt',
        '-s', '-z', entry_func
    ]

    sp.run(args, check=True, text=True)

    args = [
        'dot',
        '-Tpng', 'out.dot',
        '-o', f'perf_{quality_log}.png'
    ]

    sp.run(args, check=True, text=True)

    os.remove('out.dot')
    os.remove('callgrind.txt')


def parse_command_line() -> argparse.Namespace:
    '''
    Parse the command line.

    Return:
        The parsed command line.
    '''
    parser = argparse.ArgumentParser()

    parser.add_argument('img', type=argparse.FileType('r'),
                        help='the image file to compress')

    encoders = ['sse2', 'sse4.1', 'avx2']
    parser.add_argument('--encoder', dest='encoder', default='avx2',
                        choices=encoders,
                        help='the encoder variant to profile')

    testquant = [str(x) for x in range(0, 101, 10)]
    testqual = ['-fastest', '-fast', '-medium', '-thorough', '-exhaustive']
    qualities = testqual + testquant
    parser.add_argument('--test-quality', dest='quality', default='medium',
                        choices=qualities,
                        help='the compression quality to profile')

    parser.add_argument('--show-cli', dest='show_cli', default=False,
                        action='store_true',
                        help='enable CLI wrapper nodes in call graph')

    args = parser.parse_args()

    return args


def main() -> int:
    '''
    The main function.

    Return:
        int: The process return code.
    '''
    args = parse_command_line()
    run_pass(args.img.name, args.encoder, '6x6', args.quality, args.show_cli)
    return 0


if __name__ == '__main__':
    sys.exit(main())
