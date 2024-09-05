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
A ResultSet stores a set of results about the performance of a TestSet. Each
set keeps result Records for each image and block size tested, that store the
PSNR and coding time.

ResultSets are often backed by a CSV file on disk, and a ResultSet can be
compared against a set of reference results created by an earlier test run.
"""


import csv
import enum
import numpy as np
import os


@enum.unique
class Result(enum.IntEnum):
    """
    An enumeration of test result status values.

    Attributes:
        NOTRUN: The test has not been run.
        PASS: The test passed.
        WARN: The test image quality was below the pass threshold but above
            the fail threshold.
        FAIL: The test image quality was below the fail threshold.
    """
    NOTRUN = 0
    PASS = 1
    WARN = 2
    FAIL = 3


class ResultSummary():
    """
    An result summary data container, storing number of results of each type.

    Attributes:
        notruns: The number of tests that did not run.
        passes: The number of tests that passed.
        warnings: The number of tests that produced a warning.
        fails: The number of tests that failed.
        tTimes: Total time speedup vs reference (<1 is slower, >1 is faster).
        cTimes: Coding time speedup vs reference (<1 is slower, >1 is faster).
        psnrs: Coding time quality vs reference (<0 is worse, >0 is better).
    """

    def __init__(self):
        """
        Create a new result summary.
        """
        # Pass fail metrics
        self.notruns = 0
        self.passes = 0
        self.warnings = 0
        self.fails = 0

        # Relative results
        self.tTimesRel = []
        self.cTimesRel = []
        self.psnrRel = []

        # Absolute results
        self.cTime = []
        self.psnr = []

    def add_record(self, record):
        """
        Add a record to this summary.

        Args:
            record (Record): The Record to add.
        """
        if record.status == Result.PASS:
            self.passes += 1
        elif record.status == Result.WARN:
            self.warnings += 1
        elif record.status == Result.FAIL:
            self.fails += 1
        else:
            self.notruns += 1

        if record.tTimeRel is not None:
            self.tTimesRel.append(record.tTimeRel)
            self.cTimesRel.append(record.cTimeRel)
            self.psnrRel.append(record.psnrRel)

            self.cTime.append(record.cTime)
            self.psnr.append(record.psnr)

    def get_worst_result(self):
        """
        Get the worst result in this set.

        Returns:
            Result: The worst test result.
        """
        if self.fails:
            return Result.FAIL

        if self.warnings:
            return Result.WARN

        if self.passes:
            return Result.PASS

        return Result.NOTRUN

    def __str__(self):
        # Overall pass/fail results
        overall = self.get_worst_result().name
        dat = (overall, self.passes, self.warnings, self.fails)
        result = ["\nSet Status: %s (Pass: %u | Warn: %u | Fail: %u)" % dat]

        if (self.tTimesRel):
            # Performance summaries
            dat = (np.mean(self.tTimesRel), np.std(self.tTimesRel))
            result.append("\nTotal speed:   Mean:  %+0.3f x   Std: %0.2f x" % dat)

            dat = (np.mean(self.cTimesRel), np.std(self.cTimesRel))
            result.append("Coding speed:  Mean:  %+0.3f x   Std: %0.2f x" % dat)

            dat = (np.mean(self.psnrRel), np.std(self.psnrRel))
            result.append("Quality diff:  Mean:  %+0.3f dB  Std: %0.2f dB" % dat)

            dat = (np.mean(self.cTime), np.std(self.cTime))
            result.append("Coding time:   Mean:  %+0.3f s   Std: %0.2f s" % dat)

            dat = (np.mean(self.psnr), np.std(self.psnr))
            result.append("Quality:       Mean: %+0.3f dB  Std: %0.2f dB" % dat)

        return "\n".join(result)


class Record():
    """
    A single result record, sotring results for a singel image and block size.

    Attributes:
        blkSz: The block size.
        name: The test image name.
        psnr: The image quality (PSNR dB)
        tTime: The total compression time.
        cTime: The coding compression time.
        cRate: The coding compression rate.
        status: The test Result.
    """

    def __init__(self, blkSz, name, psnr, tTime, cTime, cRate):
        """
        Create a result record, initially in the NOTRUN status.

        Args:
            blkSz (str): The block size.
            name (str): The test image name.
            psnr (float): The image quality PSNR, in dB.
            tTime (float): The total compression time, in seconds.
            cTime (float): The coding compression time, in seconds.
            cRate (float): The coding compression rate, in MPix/s.
            tTimeRel (float): The relative total time speedup vs reference.
            cTimeRel (float): The relative coding time speedup vs reference.
            cRateRel (float): The relative rate speedup vs reference.
            psnrRel (float): The relative PSNR dB vs reference.
        """
        self.blkSz = blkSz
        self.name = name
        self.psnr = psnr
        self.tTime = tTime
        self.cTime = cTime
        self.cRate = cRate
        self.status = Result.NOTRUN

        self.tTimeRel = None
        self.cTimeRel = None
        self.cRateRel = None
        self.psnrRel = None

    def set_status(self, result):
        """
        Set the result status.

        Args:
            result (Result): The test result.
        """
        self.status = result

    def __str__(self):
        return "'%s' / '%s'" % (self.blkSz, self.name)


class ResultSet():
    """
    A set of results for a TestSet, across one or more block sizes.

    Attributes:
        testSet: The test set these results are linked to.
        records: The list of test results.
    """

    def __init__(self, testSet):
        """
        Create a new empty ResultSet.

        Args:
            testSet (TestSet): The test set these results are linked to.
        """
        self.testSet = testSet
        self.records = []

    def add_record(self, record):
        """
        Add a new test record to this result set.

        Args:
            record (Record): The test record to add.
        """
        self.records.append(record)

    def get_record(self, testSet, blkSz, name):
        """
        Get a record matching the arguments.

        Args:
            testSet (TestSet): The test set to get results from.
            blkSz (str): The block size.
            name (str): The test name.

        Returns:
            Record: The test result, if present.

        Raises:
            KeyError: No match could be found.
        """
        if testSet != self.testSet:
            raise KeyError()

        for record in self.records:
            if record.blkSz == blkSz and record.name == name:
                return record

        raise KeyError()

    def get_matching_record(self, other):
        """
        Get a record matching the config of another record.

        Args:
            other (Record): The pattern result record to match.

        Returns:
            Record: The result, if present.

        Raises:
            KeyError: No match could be found.
        """
        for record in self.records:
            if record.blkSz == other.blkSz and record.name == other.name:
                return record

        raise KeyError()

    def get_results_summary(self):
        """
        Get a results summary of all the records in this result set.

        Returns:
            ResultSummary: The result summary.
        """
        summary = ResultSummary()
        for record in self.records:
            summary.add_record(record)

        return summary

    def save_to_file(self, filePath):
        """
        Save this result set to a CSV file.

        Args:
            filePath (str): The output file path.
        """
        dirName = os.path.dirname(filePath)
        if not os.path.exists(dirName):
            os.makedirs(dirName)

        with open(filePath, "w", newline="") as csvfile:
            writer = csv.writer(csvfile)
            self._save_header(writer)
            for record in self.records:
                self._save_record(writer, record)

    @staticmethod
    def _save_header(writer):
        """
        Write the header to the CSV file.

        Args:
            writer (csv.writer): The CSV writer.
        """
        row = ["Image Set", "Block Size", "Name",
               "PSNR", "Total Time", "Coding Time", "Coding Rate"]
        writer.writerow(row)

    def _save_record(self, writer, record):
        """
        Write a record to the CSV file.

        Args:
            writer (csv.writer): The CSV writer.
            record (Record): The record to write.
        """
        row = [self.testSet,
               record.blkSz,
               record.name,
               "%0.4f" % record.psnr,
               "%0.4f" % record.tTime,
               "%0.4f" % record.cTime,
               "%0.4f" % record.cRate]
        writer.writerow(row)

    def load_from_file(self, filePath):
        """
        Load a reference result set from a CSV file on disk.

        Args:
            filePath (str): The input file path.
        """
        with open(filePath, "r") as csvfile:
            reader = csv.reader(csvfile)
            # Skip the header
            next(reader)
            for row in reader:
                assert row[0] == self.testSet
                record = Record(row[1], row[2],
                                float(row[3]), float(row[4]),
                                float(row[5]), float(row[6]))
                self.add_record(record)
