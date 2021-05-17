#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2020-2021 Arm Limited
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
The ``astc_test_result_report.py`` script consolidates all current sets of
reference results into a single report giving PSNR diffs (absolute) and
performance diffs (relative speedup, 1 = no change).
"""

import re
import os
import sys


import testlib.resultset as trs
from collections import defaultdict as ddict


CONFIG_FILTER = [
    re.compile(r"^.*1\.7.*$"),
    re.compile(r"^.*sse.*$")
]

TESTSET_FILTER = [
    re.compile(r"^Small$"),
    re.compile(r"^Frymire$"),
]

QUALITY_FILTER = [
]

BLOCKSIZE_FILTER = [
    re.compile(r"^12x12$")
]


def find_reference_results():
    """
    Scrape the Test/Images directory for result CSV files and return an
    mapping of the result sets.

    Returns:
        Returns a three deep tree of dictionaries, with the final dict
        pointing at a `ResultSet` object. The hierarchy is:

            imageSet => quality => encoder => result
    """
    scriptDir = os.path.dirname(__file__)
    imageDir = os.path.join(scriptDir, "Images")

    # Pattern for extracting useful data from the CSV file name
    filePat = re.compile(r"astc_reference-(.+)_(.+)_results\.csv")

    # Build a three level dictionary we can write into
    results = ddict(lambda: ddict(lambda: ddict()))

    # Final all CSVs, load them and store them in the dict tree
    for root, dirs, files in os.walk(imageDir):
        for name in files:
            match = filePat.match(name)
            if match:

                # Skip results set in the filter
                skip = [1 for filt in CONFIG_FILTER if filt.match(name)]
                if skip:
                    continue

                fullPath = os.path.join(root, name)

                encoder = match.group(1)
                quality = match.group(2)
                imageSet = os.path.basename(root)

                # Skip results set in the filter
                skip = [1 for filt in TESTSET_FILTER if filt.match(imageSet)]
                if skip:
                    continue

                # Skip results set in the filter
                skip = [1 for filt in QUALITY_FILTER if filt.match(quality)]
                if skip:
                    continue

                testRef = trs.ResultSet(imageSet)
                testRef.load_from_file(fullPath)

                patchedRef = trs.ResultSet(imageSet)

                for result in testRef.records:
                    skip = [1 for filt in BLOCKSIZE_FILTER if filt.match(result.blkSz)]
                    if not skip:
                        patchedRef.add_record(result)

                results[imageSet][quality]["ref-%s" % encoder] = patchedRef

    return results


class DeltaRecord():
    """
    Record a single image result from N different encoders.

    Attributes:
        imageSet: The image set this cme from.
        quality: The compressor quality used.
        encoders: The names of the encoders used. The first encoder in this
            list will be used as the reference result.
        records: The raw records for the encoders. The order of records in this
            list matches the order of the `encoders` list.
    """

    def __init__(self, imageSet, quality, encoders, records):
        self.imageSet = imageSet
        self.quality = quality

        self.encoders = list(encoders)
        self.records = list(records)

        assert(len(self.encoders) == len(self.records))

    def get_delta_header(self, tag):
        """
        Get the delta encoding header.

        Args:
            tag: The field name to include in the tag.

        Return:
            The array of strings, providing the header names.
        """
        result = []

        for encoder in self.encoders[1:]:
            result.append("%s %s" % (tag, encoder))

        return result

    def get_abs_delta(self, field):
        """
        Get an absolute delta result.

        Args:
            field: The Record attribute name to diff.

        Return:
            The array of float delta values.
        """
        result = []

        root = self.records[0]
        for record in self.records[1:]:
            result.append(getattr(record, field) - getattr(root, field))

        return result

    def get_rel_delta(self, field):
        """
        Get an relative delta result (score / ref).

        Args:
            field: The Record attribute name to diff.

        Return:
            The array of float delta values.
        """
        result = []

        root = self.records[0]
        for record in self.records[1:]:
            result.append(getattr(record, field) / getattr(root, field))

        return result

    def get_irel_delta(self, field):
        """
        Get an inverse relative delta result (ref / score).

        Args:
            field: The Record attribute name to diff.

        Return:
            The array of float delta values.
        """
        return [1.0 / x for x in self.get_rel_delta(field)]

    def get_full_row_header_csv(self):
        """
        Get a CSV encoded delta record header.

        Return:
            The string for the row.
        """
        rows = [
            "Image Set",
            "Quality",
            "Size",
            "Name"
        ]

        rows.append("")
        rows.extend(self.get_delta_header("PSNR"))

        rows.append("")
        rows.extend(self.get_delta_header("Speed"))

        return ",".join(rows)

    def get_full_row_csv(self):
        """
        Get a CSV encoded delta record.

        Return:
            The string for the row.
        """
        rows = [
            self.imageSet,
            self.quality,
            self.records[0].name,
            self.records[0].blkSz
        ]

        rows.append("")
        data = ["%0.3f" % x for x in self.get_abs_delta("psnr")]
        rows.extend(data)

        rows.append("")
        data = ["%0.3f" % x for x in self.get_irel_delta("cTime")]
        rows.extend(data)

        return ",".join(rows)


def print_result_set(imageSet, quality, encoders, results, printHeader):
    """
    Attributes:
        imageSet: The image set name.
        quality: The compressor quality used.
        encoders: The names of the encoders used. The first encoder in this
            list will be used as the reference result.
        results: The dict of results, indexed by encoder.
        printHeader: True if the table header should be printed, else False.
    """
    results = [results[x] for x in encoders]
    recordSizes = [len(x.records) for x in results]

    # Skip result sets that are not the same size
    # TODO: We can take the set intersection here to report what we can
    if min(recordSizes) != max(recordSizes):
        return

    # Interleave all result records
    recordSets = zip(*[x.records for x in results])

    # Iterate each image
    for recordSet in recordSets:
        base = recordSet[0]

        # Sanity check consistency
        for record in recordSet[1:]:
            assert(record.blkSz == base.blkSz)
            assert(record.name == base.name)

        dr = DeltaRecord(imageSet, quality, encoders, recordSet)

        if printHeader:
            print(dr.get_full_row_header_csv())
            printHeader = False

        print(dr.get_full_row_csv())


def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """

    results = find_reference_results()

    imageSet = sorted(results.keys())

    first = True
    for image in imageSet:
        qualityTree = results[image]
        qualitySet = sorted(qualityTree.keys())

        for qual in qualitySet:
            encoderTree = qualityTree[qual]
            encoderSet = sorted(encoderTree.keys())

            if len(encoderSet) > 1:
                print_result_set(image, qual, encoderSet, encoderTree, first)
                first = False

    return 0


if __name__ == "__main__":
    sys.exit(main())
