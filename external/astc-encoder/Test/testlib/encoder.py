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
These classes provide an abstraction around the astcenc command line tool,
allowing the rest of the image test suite to ignore changes in the command line
interface.

Currently this module supports the latest 2.x branch onwards; the 1.x branch
is no longer supported.

TODO:
* Refactor more heavily now that we have dropped 1.x encoder support, given
  that all the compressors are the same now ...
'''

import os
from pathlib import Path
import re
import subprocess as sp
import sys
from typing import Optional

from .testset import TestImage


RunResult = tuple[float, float, float, float]


class EncoderBase:
    '''
    A wrapper around the astcenc binary, abstracting the command line so that
    the rest of the test suite does not need to worry about changes to the
    command line interface over versions.

    This is an abstract base class providing some generic helper functionality
    used by concrete instantiations of subclasses.

    Attributes:
        name: The encoder name to use in reports.
        variant: The encoder SIMD variant being tested.
        binary_path: The encoder binary path.

    Class attributes:
        version: Encoder version or branch.
        remap_color_switches: Dict of color format to profile switches.
        remap_output_types: Dict of color format to output file extensions.
    '''

    # Subclasses override these if they need them
    version = ''
    remap_color_switches: dict[str, str] = {}
    remap_output_types: dict[str, str] = {}

    def __init__(self, name: str, variant: str, binary_path: Path):
        '''
        Create a new encoder instance.

        Args:
            name: The name of the encoder.
            variant: The SIMD variant of the encoder.
            binary: The path to the binary on the file system.
        '''
        self.name = name
        self.variant = variant
        self.binary_path = binary_path

    def build_cli(self, image: TestImage, block_size: str = '6x6',
                  preset: str = '-thorough', keep_output: bool = True,
                  threads: Optional[int] = None) -> list[str]:
        '''
        Build the command line needed for the given test.

        Args:
            image: The test image to compress.
            block_size: The block size to use.
            preset: The quality preset to use.
            keep_output: Should the test preserve output images? This is a hint
                and may be ignored if astcenc version used can't do it.
            threads: The thread count to use.

        Return:
            The command line arguments for this encoder version.
        '''
        # pylint: disable=unused-argument,redundant-returns-doc
        assert False, 'Missing subclass implementation'

    def execute(self, command: list[str]) -> list[str]:
        '''
        Run a subprocess with the specified command.

        Args:
            command: The command line arguments to use.

        Return:
            The output log (stdout) split into lines.
        '''
        try:
            result = sp.run(command, stdout=sp.PIPE, stderr=sp.PIPE,
                            check=True, text=True)

        except (OSError, sp.CalledProcessError):
            print(f'  + {" ".join(command)}')
            assert False, 'ERROR: Test run failed'

        return result.stdout.splitlines()

    def parse_output(self, image: TestImage, output: list[str]) -> RunResult:
        '''
        Parse the log output for PSNR and performance metrics.

        Args:
            image: The test image that was compressed.
            output: The astcenc compression output log.

        Return:
            Tuple containing PSNR in dB, total time in seconds, coding time
            in seconds, and coding rate in MT/s.
        '''
        # Regex patterns. provided by this particular subclass
        pattern_psnr = self.get_psnr_pattern(image)
        pattern_total_time = self.get_total_time_pattern()
        pattern_coding_time = self.get_coding_time_pattern()
        pattern_coding_rate = self.get_coding_rate_pattern()

        # Extract results from the log
        psnr = None
        total_time = None
        coding_time = None
        coding_rate = None

        for line in output:
            if match := pattern_psnr.match(line):
                psnr = float(match.group(1))
                continue

            if match := pattern_total_time.match(line):
                total_time = float(match.group(1))
                continue

            if match := pattern_coding_time.match(line):
                coding_time = float(match.group(1))
                continue

            if match := pattern_coding_rate.match(line):
                coding_rate = float(match.group(1))
                continue

        stdout = '\n'.join(output)
        assert psnr is not None, f'Missing PSNR {stdout}'
        assert total_time is not None, f'Missing total time {stdout}'
        assert coding_time is not None, f'Missing coding time {stdout}'
        assert coding_rate is not None, f'Missing coding rate {stdout}'

        return (psnr, total_time, coding_time, coding_rate)

    def get_psnr_pattern(self, image: TestImage) -> re.Pattern:
        '''
        Get the regex pattern to match the image quality metric.

        Note that, although this function is called PSNR, for some images we
        may choose to match another metric (e.g. mPSNR for HDR images).

        Args:
            image: The test image that was compressed.

        Return:
            The regex pattern.
        '''
        # pylint: disable=unused-argument,redundant-returns-doc
        assert False, 'Missing subclass implementation'
        return re.compile('^(?!x)x$')

    def get_total_time_pattern(self) -> re.Pattern:
        '''
        Get the regex pattern to match the total compression time.

        Return:
            The regex pattern.
        '''
        # pylint: disable=unused-argument,redundant-returns-doc
        assert False, 'Missing subclass implementation'
        return re.compile('^(?!x)x$')

    def get_coding_time_pattern(self) -> re.Pattern:
        '''
        Get the regex pattern to match the coding compression time.

        Return:
            The regex pattern.
        '''
        # pylint: disable=unused-argument,redundant-returns-doc
        assert False, 'Missing subclass implementation'
        return re.compile('^(?!x)x$')

    def get_coding_rate_pattern(self) -> re.Pattern:
        '''
        Get the regex pattern to match the coding rate.

        Return:
            The regex pattern.
        '''
        # pylint: disable=unused-argument,redundant-returns-doc
        assert False, 'Missing subclass implementation'
        return re.compile('^(?!x)x$')

    def run_test(self, image: TestImage, block_size: str, preset: str,
                 repeats: int, keep_output: bool = True,
                 threads: Optional[int] = None) -> RunResult:
        '''
        Run the test N times.

        Args:
            image: The test image to compress.
            block_size: The block size to use.
            preset: The quality-performance preset to use.
            repeats: The number of test runs.
            keep_output: Should the test preserve output images?
            threads: The thread count to use.

        Return:
            Tuple containing PSNR in dB, total time in seconds, coding time
            in seconds, and coding rate in MT/s.
        '''
        # pylint: disable=assignment-from-no-return
        command = self.build_cli(image, block_size, preset, keep_output,
                                 threads)

        # Execute test runs keeping best results
        best_psnr = 0.0
        best_total_time = sys.float_info.max
        best_coding_time = sys.float_info.max
        best_coding_rate = 0.0

        for _ in range(0, repeats):
            output = self.execute(command)
            result = self.parse_output(image, output)

            # Keep the best results (highest PSNR, lowest times, highest rate)
            best_psnr = max(best_psnr, result[0])
            best_total_time = min(best_total_time, result[1])
            best_coding_time = min(best_coding_time, result[2])
            best_coding_rate = max(best_coding_rate, result[3])

        return (best_psnr, best_total_time, best_coding_time, best_coding_rate)


class Encoder2x(EncoderBase):
    '''
    This class wraps the latest astcenc interface, supported from the 2.0
    release or later, which broke argument compatibility with the earlier
    1.x series.
    '''
    version = 'main'

    remap_color_switches = {
        'ldr': '-tl',
        'ldrs': '-ts',
        'hdr': '-th',
        'hdra': '-tH'
    }

    remap_output_types = {
        'ldr': '.png',
        'ldrs': '.png',
        'hdr': '.exr',
        'hdra': '.exr'
    }

    def __init__(self, variant: str, binary_path: Optional[Path] = None):
        name = f'astcenc-{variant}-{self.version}'

        if binary_path is None:
            binary = 'astcenc'
            post = f'-{variant}' if variant != 'universal' else ''
            ext = '.exe' if os.name == 'nt' else ''
            binary_path = Path('./') / 'bin' / f'{binary}{post}{ext}'

        super().__init__(name, variant, binary_path)

    def build_cli(self, image, block_size='6x6', preset='-thorough',
                  keep_output=True, threads=None):
        operation = self.remap_color_switches[image.color_profile]

        src_path = image.file_path

        if keep_output:
            extension = self.remap_output_types[image.color_profile]

            dst_file = f'{image.file_path_out.stem}{extension}'

            dst_dir = image.file_path_out.parent
            dst_dir = dst_dir / self.name / preset[1:] / block_size
            dst_dir.mkdir(parents=True, exist_ok=True)

            dst_path = str(dst_dir / dst_file)

        else:
            if sys.platform == 'win32':
                dst_path = 'nul'
            else:
                dst_path = '/dev/null'

        command = [
            str(self.binary_path), operation, str(src_path), str(dst_path),
            block_size, preset, '-silent'
        ]

        if image.color_format == 'xy':
            command.append('-normal')

        if image.is_alpha_scaled:
            command.append('-a')
            command.append('1')

        if threads is not None:
            command.append('-j')
            command.append(f'{threads}')

        return command

    def get_psnr_pattern(self, image: TestImage) -> re.Pattern:
        # HDR profile
        if image.color_profile == 'hdr':
            pattern_psnr = r'\s*mPSNR \(RGB\)(?: \[.*?\] )?:\s*([0-9.]*) dB.*'
        # LDR profile color formats
        elif image.color_format == 'rgba':
            pattern_psnr = r'\s*PSNR \(LDR-RGBA\):\s*([0-9.]*) dB'
        else:
            pattern_psnr = r'\s*PSNR \(LDR-RGB\):\s*([0-9.]*) dB'

        return re.compile(pattern_psnr)

    def get_total_time_pattern(self) -> re.Pattern:
        return re.compile(r'\s*Total time:\s*([0-9.]*) s')

    def get_coding_time_pattern(self) -> re.Pattern:
        return re.compile(r'\s*Coding time:\s*([0-9.]*) s')

    def get_coding_rate_pattern(self) -> re.Pattern:
        return re.compile(r'\s*Coding rate:\s*([0-9.]*) MT/s')


class Encoder2xRel(Encoder2x):
    '''
    This class wraps a released 2.x series binary.
    '''
    def __init__(self, version: str, variant: str):

        self.version = version

        binary = 'astcenc'
        post = f'-{variant}' if variant != 'universal' else ''
        ext = '.exe' if os.name == 'nt' else ''
        binary_path = Path('./') / 'Binaries' / f'{binary}{post}{ext}'

        super().__init__(variant, binary_path)
