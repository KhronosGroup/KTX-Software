#! /usr/bin/env python3
# Copyright 2026 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Since there are a lot of files in the CLI test suite (CTS), keeping track of
# files that are no longer used (especially during large PR development) can be
# hard.
#
# This tool takes an `strace` logfile, filters referenced files in
# `clitests/golden` or `clitests/input` subdirectories, and prints
# unused/unreferenced files.
#
# You can generate the strace log file as such:
#   strace -f -e trace=openat -o files.trace su user -c 'ctest -j1'
# Note:
#   - Make sure to run CTest in single-threaded mode otherwise you will get
#     extremely large trace files with a lot of "unfinished" syscalls.

import argparse
import os
import re

def parse_strace_filename(filename: str):
    s = re.sub(r'\\([0-7]{1,3})', lambda m: chr(int(m.group(1), 8)), filename)
    return s.encode("latin-1").decode("utf-8");

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Print unreferenced (and potentially not needed) CTS files from provided strace logfile",
        usage="cts_list_unused_files.py <strace-logfile> <clitests-dir> [<args>]")
    parser.add_argument('strace_logfile', help="Path to strace log file")
    parser.add_argument('clitests_dir', help="Path to clitests directory")
    parser.add_argument('-i', '--input', action='store_true', help='Compare to `clitests/input` subdirectory instead of `clitests/golden`')
    args = parser.parse_args()

    if not os.path.isfile(args.strace_logfile):
        print(f"ERROR: Cannot find provided strace logfile '{args.strace_logfile}'")
        exit(1)

    if not os.path.isdir(args.clitests_dir):
        print(f"ERROR: Cannot find provided clitest directory '{args.clitests_dir}'")
        exit(1)

    f = open(args.strace_logfile, encoding='utf-8')
    # find all referenced files in CTS subdirectory and save them in a hash list
    referenced_files = set()

    # Example:
    # 226232 openat(AT_FDCWD, "input/ktx2/valid_ASTC_6x6_SRGB_BLOCK_2D.ktx2", O_RDONLY) = 3
    subdir_name = "input" if args.input else "golden"
    for l in f:
        start_idx = l.find('"')
        if start_idx == -1 or start_idx + 1 >= len(l):
            continue
        # Example:
        # golden/extract/png_2d/output_ASTC_10x8_SRGB_BLOCK_2D.png
        if not l.startswith(subdir_name, start_idx + 1):
            continue
        end_idx = l.find('"', start_idx + 1)
        if end_idx == -1:
            continue;
        # Strace may print utf-8 non-ascii chars as octal binary; parse these to proper utf-8
        p = parse_strace_filename(os.path.join(args.clitests_dir, l[start_idx + 1:end_idx]))
        if not os.path.isfile(p):
            # It is expected that missing_file test case actually does not exist
            if "missing_file" in p:
                continue
            print(f"ERROR: cannot find referenced filename {p}. Either provided clitests is not an actual 'clitests' directory with expected subdirs or reference file no longer exists.")
            exit(1)
        referenced_files.add(p)

    nbr_unused_files = 0
    total_size = 0
    for path, subdirs, files in os.walk(os.path.join(args.clitests_dir, subdir_name)):
        for name in files:
            p = os.path.join(path, name)
            if p not in referenced_files:
                nbr_unused_files += 1
                total_size += os.stat(p).st_size
                print(p)

    if nbr_unused_files:
        print(f"number of unused files in '{subdir_name}' subdirectory: {nbr_unused_files}")
        print(f"total size of unused files in '{subdir_name}' subdirectory (MiB): {total_size / (1024 * 1024)}")
