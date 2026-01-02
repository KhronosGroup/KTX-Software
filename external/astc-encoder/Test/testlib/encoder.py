# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2019-2023 Arm Limited
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
These classes provide an abstraction around the astcenc command line tool,
allowing the rest of the image test suite to ignore changes in the command line
interface.
"""

import os
import re
import subprocess as sp
import sys


class EncoderBase():
    """
    This class is a Python wrapper for the `astcenc` binary, providing an
    abstract means to set command line options and parse key results.

    This is an abstract base class providing some generic helper functionality
    used by concrete instantiations of subclasses.

    Attributes:
        binary: The encoder binary path.
        variant: The encoder SIMD variant being tested.
        name: The encoder name to use in reports.
        VERSION: The encoder version or branch.
        SWITCHES: Dict of switch replacements for different color formats.
        OUTPUTS: Dict of output file extensions for different color formats.
    """

    VERSION = None
    SWITCHES = None
    OUTPUTS = None

    def __init__(self, name, variant, binary):
        """
        Create a new encoder instance.

        Args:
            name (str): The name of the encoder.
            variant (str): The SIMD variant of the encoder.
            binary (str): The path to the binary on the file system.
        """
        self.name = name
        self.variant = variant
        self.binary = binary

    def build_cli(self, image, blockSize="6x6", preset="-thorough",
                  keepOutput=True, threads=None):
        """
        Build the command line needed for the given test.

        Args:
            image (TestImage): The test image to compress.
            blockSize (str): The block size to use.
            preset (str): The quality-performance preset to use.
            keepOutput (bool): Should the test preserve output images? This is
                only a hint and discarding output may be ignored if the encoder
                version used can't do it natively.
            threads (int or None): The thread count to use.

        Returns:
            list(str): A list of command line arguments.
        """
        # pylint: disable=unused-argument,no-self-use,redundant-returns-doc
        assert False, "Missing subclass implementation"

    def execute(self, command):
        """
        Run a subprocess with the specified command.

        Args:
            command (list(str)): The list of command line arguments.

        Returns:
            list(str): The output log (stdout) split into lines.
        """
        # pylint: disable=no-self-use
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

    def parse_output(self, image, output):
        """
        Parse the log output for PSNR and performance metrics.

        Args:
            image (TestImage): The test image to compress.
            output (list(str)): The output log from the compression process.

        Returns:
            tuple(float, float, float): PSNR in dB, TotalTime in seconds, and
            CodingTime in seconds.
        """
        # Regex pattern for image quality
        patternPSNR = re.compile(self.get_psnr_pattern(image))
        patternTTime = re.compile(self.get_total_time_pattern())
        patternCTime = re.compile(self.get_coding_time_pattern())
        patternCRate = re.compile(self.get_coding_rate_pattern())

        # Extract results from the log
        runPSNR = None
        runTTime = None
        runCTime = None
        runCRate = None

        for line in output:
            match = patternPSNR.match(line)
            if match:
                runPSNR = float(match.group(1))

            match = patternTTime.match(line)
            if match:
                runTTime = float(match.group(1))

            match = patternCTime.match(line)
            if match:
                runCTime = float(match.group(1))

            match = patternCRate.match(line)
            if match:
                runCRate = float(match.group(1))

        stdout = "\n".join(output)
        assert runPSNR is not None, "No coding PSNR found %s" % stdout
        assert runTTime is not None, "No total time found %s" % stdout
        assert runCTime is not None, "No coding time found %s" % stdout
        assert runCRate is not None, "No coding rate found %s" % stdout
        return (runPSNR, runTTime, runCTime, runCRate)

    def get_psnr_pattern(self, image):
        """
        Get the regex pattern to match the image quality metric.

        Note, while this function is called PSNR for some images we may choose
        to match another metric (e.g. mPSNR for HDR images).

        Args:
            image (TestImage): The test image we are compressing.

        Returns:
            str: The string for a regex pattern.
        """
        # pylint: disable=unused-argument,no-self-use,redundant-returns-doc
        assert False, "Missing subclass implementation"

    def get_total_time_pattern(self):
        """
        Get the regex pattern to match the total compression time.

        Returns:
            str: The string for a regex pattern.
        """
        # pylint: disable=unused-argument,no-self-use,redundant-returns-doc
        assert False, "Missing subclass implementation"

    def get_coding_time_pattern(self):
        """
        Get the regex pattern to match the coding compression time.

        Returns:
            str: The string for a regex pattern.
        """
        # pylint: disable=unused-argument,no-self-use,redundant-returns-doc
        assert False, "Missing subclass implementation"

    def run_test(self, image, blockSize, preset, testRuns, keepOutput=True,
                 threads=None):
        """
        Run the test N times.

        Args:
            image (TestImage): The test image to compress.
            blockSize (str): The block size to use.
            preset (str): The quality-performance preset to use.
            testRuns (int): The number of test runs.
            keepOutput (bool): Should the test preserve output images? This is
                only a hint and discarding output may be ignored if the encoder
                version used can't do it natively.
            threads (int or None): The thread count to use.

        Returns:
            tuple(float, float, float, float): Returns the best results from
            the N test runs, as PSNR (dB), total time (seconds), coding time
            (seconds), and coding rate (M pixels/s).
        """
        # pylint: disable=assignment-from-no-return
        command = self.build_cli(image, blockSize, preset, keepOutput, threads)

        # Execute test runs
        bestPSNR = 0
        bestTTime = sys.float_info.max
        bestCTime = sys.float_info.max
        bestCRate = 0

        for _ in range(0, testRuns):
            output = self.execute(command)
            result = self.parse_output(image, output)

            # Keep the best results (highest PSNR, lowest times, highest rate)
            bestPSNR = max(bestPSNR, result[0])
            bestTTime = min(bestTTime, result[1])
            bestCTime = min(bestCTime, result[2])
            bestCRate = max(bestCRate, result[3])

        return (bestPSNR, bestTTime, bestCTime, bestCRate)


class Encoder2x(EncoderBase):
    """
    This class wraps the latest `astcenc` 2.x series binaries from main branch.
    """
    VERSION = "main"

    SWITCHES = {
        "ldr": "-tl",
        "ldrs": "-ts",
        "hdr": "-th",
        "hdra": "-tH"
    }

    OUTPUTS = {
        "ldr": ".png",
        "ldrs": ".png",
        "hdr": ".exr",
        "hdra": ".exr"
    }

    def __init__(self, variant, binary=None):
        name = "astcenc-%s-%s" % (variant, self.VERSION)

        if binary is None:
            if variant != "universal":
                binary = f"./bin/astcenc-{variant}"
            else:
                binary = "./bin/astcenc"

            if os.name == 'nt':
                binary = f"{binary}.exe"

        super().__init__(name, variant, binary)

    def build_cli(self, image, blockSize="6x6", preset="-thorough",
                  keepOutput=True, threads=None):
        opmode = self.SWITCHES[image.colorProfile]
        srcPath = image.filePath

        if keepOutput:
            dstPath = image.outFilePath + self.OUTPUTS[image.colorProfile]
            dstDir = os.path.dirname(dstPath)
            dstFile = os.path.basename(dstPath)
            dstPath = os.path.join(dstDir, self.name, preset[1:], blockSize, dstFile)

            dstDir = os.path.dirname(dstPath)
            os.makedirs(dstDir, exist_ok=True)
        elif sys.platform == "win32":
            dstPath = "nul"
        else:
            dstPath = "/dev/null"

        command = [
            self.binary, opmode, srcPath, dstPath,
            blockSize, preset, "-silent"
        ]

        if image.colorFormat == "xy":
            command.append("-normal")

        if image.isAlphaScaled:
            command.append("-a")
            command.append("1")

        if threads is not None:
            command.append("-j")
            command.append("%u" % threads)

        return command

    def get_psnr_pattern(self, image):
        if image.colorProfile != "hdr":
            if image.colorFormat != "rgba":
                patternPSNR = r"\s*PSNR \(LDR-RGB\):\s*([0-9.]*) dB"
            else:
                patternPSNR = r"\s*PSNR \(LDR-RGBA\):\s*([0-9.]*) dB"
        else:
            patternPSNR = r"\s*mPSNR \(RGB\)(?: \[.*?\] )?:\s*([0-9.]*) dB.*"
        return patternPSNR

    def get_total_time_pattern(self):
        return r"\s*Total time:\s*([0-9.]*) s"

    def get_coding_time_pattern(self):
        return r"\s*Coding time:\s*([0-9.]*) s"

    def get_coding_rate_pattern(self):
        return r"\s*Coding rate:\s*([0-9.]*) MT/s"


class Encoder2xRel(Encoder2x):
    """
    This class wraps a released 2.x series binary.
    """
    def __init__(self, version, variant):

        self.VERSION = version

        if variant != "universal":
            binary = f"./Binaries/{version}/astcenc-{variant}"
        else:
            binary = f"./Binaries/{version}/astcenc"

        if os.name == 'nt':
            binary = f"{binary}.exe"

        super().__init__(variant, binary)


class Encoder1_7(EncoderBase):
    """
    This class wraps the 1.7 series binaries.
    """
    VERSION = "1.7"

    SWITCHES = {
        "ldr": "-tl",
        "ldrs": "-ts",
        "hdr": "-t"
    }

    OUTPUTS = {
        "ldr": ".tga",
        "ldrs": ".tga",
        "hdr": ".htga"
    }

    def __init__(self):
        name = "astcenc-%s" % self.VERSION
        if os.name == 'nt':
            binary = "./Binaries/1.7/astcenc.exe"
        else:
            binary = "./Binaries/1.7/astcenc"

        super().__init__(name, None, binary)

    def build_cli(self, image, blockSize="6x6", preset="-thorough",
                  keepOutput=True, threads=None):

        if preset == "-fastest":
            preset = "-fast"

        opmode = self.SWITCHES[image.colorProfile]
        srcPath = image.filePath

        dstPath = image.outFilePath + self.OUTPUTS[image.colorProfile]
        dstDir = os.path.dirname(dstPath)
        dstFile = os.path.basename(dstPath)
        dstPath = os.path.join(dstDir, self.name, preset[1:], blockSize, dstFile)

        dstDir = os.path.dirname(dstPath)
        os.makedirs(dstDir, exist_ok=True)

        command = [
            self.binary, opmode, srcPath, dstPath,
            blockSize, preset, "-silentmode", "-time", "-showpsnr"
        ]

        if image.colorFormat == "xy":
            command.append("-normal_psnr")

        if image.colorProfile == "hdr":
            command.append("-hdr")

        if image.isAlphaScaled:
            command.append("-alphablend")

        if threads is not None:
            command.append("-j")
            command.append("%u" % threads)

        return command

    def get_psnr_pattern(self, image):
        if image.colorProfile != "hdr":
            if image.colorFormat != "rgba":
                patternPSNR = r"PSNR \(LDR-RGB\):\s*([0-9.]*) dB"
            else:
                patternPSNR = r"PSNR \(LDR-RGBA\):\s*([0-9.]*) dB"
        else:
            patternPSNR = r"mPSNR \(RGB\)(?: \[.*?\] )?:\s*([0-9.]*) dB.*"
        return patternPSNR

    def get_total_time_pattern(self):
        # Pattern match on a new pattern for a 2.1 compatible variant
        # return r"Elapsed time:\s*([0-9.]*) seconds.*"
        return r"\s*Total time:\s*([0-9.]*) s"

    def get_coding_time_pattern(self):
        # Pattern match on a new pattern for a 2.1 compatible variant
        # return r".* coding time: \s*([0-9.]*) seconds"
        return r"\s*Coding time:\s*([0-9.]*) s"

    def get_coding_rate_pattern(self):
        # Pattern match on a new pattern for a 2.1 compatible variant
        return r"\s*Coding rate:\s*([0-9.]*) MT/s"
