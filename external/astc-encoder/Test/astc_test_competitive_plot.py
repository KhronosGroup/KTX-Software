#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2022 Arm Limited
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
This script is a simple test result plotter for sweeps on multiple compressors.
"""
import csv
import numpy as np
import matplotlib.pyplot as plt
import sys

DATABASE = "competitive.csv"


class Series:

    def __init__(self, name, perf, qual):
        self.name = name
        self.perf = perf
        self.qual = qual


def get_series(database, compressor, quality, block_size):
    title = f"{compressor} {quality} {block_size}"
    in_section = False

    perf = []
    qual = []

    with open(database) as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            if len(row) == 1:
                in_section = row[0] == title
                continue

            if in_section:
                perf.append(float(row[2]))
                qual.append(float(row[3]))

    return (perf, qual)


def plot(block_size, series_set):

    for series in series_set:
        plt.scatter(series.perf, series.qual, s=2, label=series.name)

    plt.xlabel("Speed (MT/s)")
    plt.ylabel("PSNR dB")
    plt.legend(loc='lower right', prop={'size': 6})

    plt.tight_layout()
    plt.savefig(f"ASTC_v_ISPC_{block_size}.png")
    plt.clf()


def plot_diff(series_a, series_b):

    diff_perf = np.divide(series_a.perf, series_b.perf)
    diff_qual = np.subtract(series_a.qual, series_b.qual)
    label = f"{series_a.name} vs {series_b.name}"

    plt.scatter(diff_perf, diff_qual, s=2, c="#0091BD", label=label)
    plt.scatter(np.mean(diff_perf), np.mean(diff_qual), s=10, c="#FFA500", marker="*")

    plt.axhline(y=0, color="r", linestyle="dotted", lw=0.5)
    plt.axvline(x=1, color="r", linestyle="dotted", lw=0.5)

    plt.xlabel("Relative speed")
    plt.ylabel("PSNR diff (dB)")
    plt.legend(loc='lower right', prop={'size': 6})

    plt.tight_layout()
    file_name = label.replace(" ", "_") + ".png"
    plt.savefig(file_name)
    plt.clf()


def main():

    block_sizes = ["4x4", "6x6", "8x8"]

    for block_size in block_sizes:
        series_set = []

        perf, qual = get_series(DATABASE, "ISPC", "rgba", block_size)
        series_set.append(Series(f"{block_size} IPSC Slow", perf, qual))

        perf, qual = get_series(DATABASE, "ISPC", "rgb", block_size)
        series_set.append(Series(f"{block_size} IPSC Fast", perf, qual))

        perf, qual = get_series(DATABASE, "ASTC", "60", block_size)
        series_set.append(Series(f"{block_size} ASTC 60", perf, qual))

        perf, qual = get_series(DATABASE, "ASTC", "50", block_size)
        series_set.append(Series(f"{block_size} ASTC 50", perf, qual))

        perf, qual = get_series(DATABASE, "ASTC", "10", block_size)
        series_set.append(Series(f"{block_size} ASTC 10", perf, qual))

        perf, qual = get_series(DATABASE, "ASTC", "8", block_size)
        series_set.append(Series(f"{block_size} ASTC 8", perf, qual))

        plot(block_size, series_set)

        plot_diff(series_set[3], series_set[0])

    return 0


if __name__ == "__main__":
    sys.exit(main())
