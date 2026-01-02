#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2021 Arm Limited
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
The ``astc_trace_analysis`` utility provides a tool to analyze trace files.

WARNING: Trace files are an engineering tool, and not part of the standard
product, so traces and their associated tools are volatile and may change
significantly without notice.
"""

import argparse
from collections import defaultdict as ddict
import json
import numpy as np
import sys

QUANT_TABLE = {
	 0:   2,
	 1:   3,
	 2:   4,
	 3:   5,
	 4:   6,
	 5:   8,
	 6:  10,
	 7:  12,
	 8:  16,
	 9:  20,
	10:  24,
	11:  32
}

CHANNEL_TABLE = {
	 0: "R",
	 1: "G",
	 2: "B",
	 3: "A"
}

class Trace:

    def __init__(self, block_x, block_y, block_z):
        self.block_x = block_x
        self.block_y = block_y
        self.block_z = block_z
        self.blocks = []

    def add_block(self, block):
        self.blocks.append(block)

    def __getitem__(self, i):
       return self.blocks[i]

    def __delitem__(self, i):
       del self.blocks[i]

    def __len__(self):
       return len(self.blocks)

class Block:

    def __init__(self, pos_x, pos_y, pos_z, error_target):
        self.pos_x = pos_x
        self.pos_y = pos_y
        self.pos_z = pos_z

        self.raw_min = None
        self.raw_max = None

        self.ldr_min = None
        self.ldr_max = None

        self.error_target = error_target
        self.passes = []
        self.qualityHit = None

    def add_minimums(self, r, g, b, a):
        self.raw_min = (r, g, b, a)

        def ldr(x):
            cmax = 65535.0
            return int((r / cmax) * 255.0)

        self.ldr_min = (ldr(r), ldr(g), ldr(b), ldr(a))

    def add_maximums(self, r, g, b, a):
        self.raw_max = (r, g, b, a)

        def ldr(x):
            cmax = 65535.0
            return int((r / cmax) * 255.0)

        self.ldr_max = (ldr(r), ldr(g), ldr(b), ldr(a))

    def add_pass(self, pas):
        self.passes.append(pas)

    def __getitem__(self, i):
       return self.passes[i]

    def __delitem__(self, i):
       del self.passes[i]

    def __len__(self):
       return len(self.passes)


class Pass:

    def __init__(self, partitions, partition, planes, target_hit, mode, component):
        self.partitions = partitions
        self.partition_index = 0 if partition is None else partition
        self.planes = planes
        self.plane2_component = component
        self.target_hit = target_hit
        self.search_mode = mode
        self.candidates = []

    def add_candidate(self, candidate):
        self.candidates.append(candidate)

    def __getitem__(self, i):
       return self.candidates[i]

    def __delitem__(self, i):
       del self.candidates[i]

    def __len__(self):
       return len(self.candidates)


class Candidate:

    def __init__(self, weight_x, weight_y, weight_z, weight_quant):
        self.weight_x = weight_x
        self.weight_y = weight_y
        self.weight_z = weight_z
        self.weight_quant = weight_quant
        self.refinement_errors = []

    def add_refinement(self, errorval):
        self.refinement_errors.append(errorval)


def get_attrib(data, name, multiple=False, hard_fail=True):
    results = []
    for attrib in data:
        if len(attrib) == 2 and attrib[0] == name:
            results.append(attrib[1])

    if not results:
        if hard_fail:
            print(json.dumps(data, indent=2))
            assert False, "Attribute %s not found" % name
        if multiple:
            return list()
        return None

    if not multiple:
        if len(results) > 1:
            print(json.dumps(data, indent=2))
            assert False, "Attribute %s found %u times" % (name, len(results))
        return results[0]

    return results


def rev_enumerate(seq):
    return zip(reversed(range(len(seq))), reversed(seq))

def foreach_block(data):

    for block in data:
        yield block

def foreach_pass(data):

    for block in data:
        for pas in block:
            yield (block, pas)

def foreach_candidate(data):

    for block in data:
        for pas in block:
            # Special case - None candidates for 0 partition
            if not len(pas):
                yield (block, pas, None)

            for candidate in pas:
                yield (block, pas, candidate)

def get_node(data, name, multiple=False, hard_fail=True):
    results = []
    for attrib in data:
        if len(attrib) == 3 and attrib[0] == "node" and attrib[1] == name:
            results.append(attrib[2])

    if not results:
        if hard_fail:
            print(json.dumps(data, indent=2))
            assert False, "Node %s not found" % name
        return None

    if not multiple:
        if len(results) > 1:
            print(json.dumps(data, indent=2))
            assert False, "Node %s found %u times" % (name, len(results))
        return results[0]

    return results


def find_best_pass_and_candidate(block):
    explicit_pass = None

    best_error = 1e30
    best_pass = None
    best_candidate = None

    for pas in block:
        # Special case for constant color blocks - no trial candidates
        if pas.target_hit and pas.partitions == 0:
            return (pas, None)

        for candidate in pas:
            errorval = candidate.refinement_errors[-1]
            if errorval <= best_error:
                best_error = errorval
                best_pass = pas
                best_candidate = candidate

    # Every other return type must have both best pass and best candidate
    assert (best_pass and best_candidate)
    return (best_pass, best_candidate)


def generate_database(data):
    # Skip header
    assert(data[0] == "node")
    assert(data[1] == "root")
    data = data[2]

    bx = get_attrib(data, "block_x")
    by = get_attrib(data, "block_y")
    bz = get_attrib(data, "block_z")
    dbStruct = Trace(bx, by, bz)

    for block in get_node(data, "block", True):
        px = get_attrib(block, "pos_x")
        py = get_attrib(block, "pos_y")
        pz = get_attrib(block, "pos_z")

        minr = get_attrib(block, "min_r")
        ming = get_attrib(block, "min_g")
        minb = get_attrib(block, "min_b")
        mina = get_attrib(block, "min_a")

        maxr = get_attrib(block, "max_r")
        maxg = get_attrib(block, "max_g")
        maxb = get_attrib(block, "max_b")
        maxa = get_attrib(block, "max_a")

        et = get_attrib(block, "tune_error_threshold")

        blockStruct = Block(px, py, pz, et)
        blockStruct.add_minimums(minr, ming, minb, mina)
        blockStruct.add_maximums(maxr, maxg, maxb, maxa)
        dbStruct.add_block(blockStruct)

        for pas in get_node(block, "pass", True):
            # Don't copy across passes we skipped due to heuristics
            skipped = get_attrib(pas, "skip", False, False)
            if skipped:
                continue

            prts = get_attrib(pas, "partition_count")
            prti = get_attrib(pas, "partition_index", False, False)
            plns = get_attrib(pas, "plane_count")
            chan = get_attrib(pas, "plane_component", False, plns > 2)
            mode = get_attrib(pas, "search_mode", False, False)
            ehit = get_attrib(pas, "exit", False, False) == "quality hit"

            passStruct = Pass(prts, prti, plns, ehit, mode, chan)
            blockStruct.add_pass(passStruct)

            # Constant color blocks don't have any candidates
            if prts == 0:
                continue

            for candidate in get_node(pas, "candidate", True):
                # Don't copy across candidates we couldn't encode
                failed = get_attrib(candidate, "failed", False, False)
                if failed:
                    continue

                wx = get_attrib(candidate, "weight_x")
                wy = get_attrib(candidate, "weight_y")
                wz = get_attrib(candidate, "weight_z")
                wq = QUANT_TABLE[get_attrib(candidate, "weight_quant")]
                epre = get_attrib(candidate, "error_prerealign", True, False)
                epst = get_attrib(candidate, "error_postrealign", True, False)

                candStruct = Candidate(wx, wy, wz, wq)
                passStruct.add_candidate(candStruct)
                for value in epre:
                    candStruct.add_refinement(value)
                for value in epst:
                    candStruct.add_refinement(value)

    return dbStruct


def filter_database(data):

    for block in data:
        best_pass, best_candidate = find_best_pass_and_candidate(block)

        for i, pas in rev_enumerate(block):
            if pas != best_pass:
                del block[i]
                continue

            if best_candidate is None:
                continue

            for j, candidate in rev_enumerate(pas):
                if candidate != best_candidate:
                    del pas[j]


def generate_pass_statistics(data):
    pass


def generate_feature_statistics(data):
    # -------------------------------------------------------------------------
    # Config
    print("Compressor Config")
    print("=================")

    if data.block_z > 1:
        dat = (data.block_x, data.block_y, data.block_z)
        print("  - Block size: %ux%ux%u" % dat)
    else:
        dat = (data.block_x, data.block_y)
        print("  - Block size: %ux%u" % dat)

    print("")

    # -------------------------------------------------------------------------
    # Block metrics
    result = ddict(int)

    RANGE_QUANT = 16

    for block in foreach_block(data):
        ranges = []
        for i in range(0, 4):
            ranges.append(block.ldr_max[i] - block.ldr_min[i])

        max_range = max(ranges)
        max_range = int(max_range / RANGE_QUANT) * RANGE_QUANT

        result[max_range] += 1

    print("Channel Range")
    print("=============")
    keys = sorted(result.keys())
    for key in keys:
        dat = (key, key + RANGE_QUANT - 1, result[key])
        print("  - %3u-%3u dynamic range = %6u blocks" % dat)

    print("")

    # -------------------------------------------------------------------------
    # Partition usage
    result_totals = ddict(int)
    results = ddict(lambda: ddict(int))

    for _, pas in foreach_pass(data):
        result_totals[pas.partitions] += 1
        results[pas.partitions][pas.partition_index] += 1

    print("Partition Count")
    print("===============")
    keys = sorted(result_totals.keys())
    for key in keys:
        dat = (key, result_totals[key], len(results[key]))
        print("  - %u partition(s) = %6u blocks / %4u indicies" % dat)

    print("")

    # -------------------------------------------------------------------------
    # Plane usage
    result_count = ddict(lambda: ddict(int))
    result_channel = ddict(lambda: ddict(int))

    for _, pas in foreach_pass(data):
        result_count[pas.partitions][pas.planes] += 1
        if (pas.planes > 1):
            result_channel[pas.partitions][pas.plane2_component] += 1

    print("Plane Usage")
    print("===========")
    keys = sorted(result_count.keys())
    for key in keys:
        keys2 = sorted(result_count[key])
        for key2 in keys2:
            val2 = result_count[key][key2]
            dat = (key, key2, val2)
            print("  - %u partition(s) %u plane(s) = %6u blocks" % dat)
            if key2 == 2:
                keys3 = sorted(result_channel[key])
                for key3 in keys3:
                    dat = (CHANNEL_TABLE[key3], result_channel[key][key3])
                    print("    - %s plane                 = %6u blocks" % dat)

    print("")

    # -------------------------------------------------------------------------
    # Decimation usage
    decim_count = ddict(lambda: ddict(int))
    quant_count = ddict(lambda: ddict(lambda: ddict(int)))


    MERGE_ROTATIONS = True

    for _, pas, can in foreach_candidate(data):
        # Skip constant color blocks
        if can is None:
            continue

        wx = can.weight_x
        wy = can.weight_y

        if MERGE_ROTATIONS and wx < wy:
            wx, wy = wy, wx

        decim_count[wx][wy] += 1
        quant_count[wx][wy][can.weight_quant] += 1

    print("Decimation Usage")
    print("================")

    if MERGE_ROTATIONS:
        print("  - Note: data merging grid rotations")

    x_keys = sorted(decim_count.keys())
    for x_key in x_keys:
        y_keys = sorted(decim_count[x_key])

        for y_key in y_keys:
            count = decim_count[x_key][y_key]
            dat = (x_key, y_key, count)
            print("  - %ux%u weights      = %6u blocks" % dat)

            q_keys = sorted(quant_count[x_key][y_key])
            for q_key in q_keys:
                count = quant_count[x_key][y_key][q_key]
                dat = (q_key, count)
                print("    - %2u quant range = %6u blocks" % dat)

    print("")

    # -------------------------------------------------------------------------
    # Refinement usage

    total_count = 0
    better_count = 0
    could_have_count = 0
    success_count = 0

    refinement_step = []

    for block, pas, candidate in foreach_candidate(data):
        # Ignore zero partition blocks - they don't use refinement
        if not candidate:
            continue

        target_error = block.error_target
        start_error = candidate.refinement_errors[0]
        end_error = candidate.refinement_errors[-1]

        rpf = float(start_error - end_error) / float(len(candidate.refinement_errors))
        rpf = abs(rpf)
        refinement_step.append(rpf / start_error)

        total_count += 1
        if end_error <= start_error:
            better_count += 1

        if end_error <= target_error:
            success_count += 1
        else:
            for refinement in candidate.refinement_errors:
                if refinement <= target_error:
                    could_have_count += 1
                    break


    print("Refinement Usage")
    print("================")
    print("  - %u refinements(s)" % total_count)
    print("  - %u refinements(s) improved" % better_count)
    print("  - %u refinements(s) worsened" % (total_count - better_count))
    print("  - %u refinements(s) could hit target, but didn't" % could_have_count)
    print("  - %u refinements(s) hit target" % success_count)
    print("  - %f mean step improvement" % np.mean(refinement_step))


def parse_command_line():
    """
    Parse the command line.

    Returns:
        Namespace: The parsed command line container.
    """
    parser = argparse.ArgumentParser()

    parser.add_argument("trace", type=argparse.FileType("r"),
                        help="The trace file to analyze")

    return parser.parse_args()


def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """
    args = parse_command_line()

    data = json.load(args.trace)
    db = generate_database(data)
    filter_database(db)

    generate_feature_statistics(db)

    return 0


if __name__ == "__main__":
    sys.exit(main())
