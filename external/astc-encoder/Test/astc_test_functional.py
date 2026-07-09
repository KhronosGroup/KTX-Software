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
The functional test runner is a set of tests that validate that the astcenc
cmd line tool interface is correctly handled, under both valid and invalid
usage scenarios. These tests do not directly validate the correctness of the
core codec library, and only the options accessible via the cmd line tool
will be exercised.

Python PIL does not support enough image formats so, due to the need to
validate pixel colors in test images for both LDR and HDR images, these tests
rely on an HDRI-enabled build of ImageMagick being available on the system
path. To test if the version of ImageMagick on your system is HDRI-enabled run:

    convert --version (Linux or macOS)
    magick convert --version (Windows)

... and check that the string 'HDRI' is present in the listed features.

Test Tiles
==========

Some of these tests use small test images with defined colors. These test tiles
are 8x8 texels in size, built up from 4x4 texel blocks of solid color. When
compressing with the 4x4 ASTC block size, each 4x4 block can be represented
exactly, so we can test for specific colors in tests even though the format is
notionally lossy.

* See ASTCENC_TEST_PATTERN_LDR for the LDR test pattern colors.
* See ASTCENC_TEST_PATTERN_HDR for the HDR test pattern colors.
'''

# pylint: disable=too-many-lines

import argparse
import filecmp
import os
from pathlib import Path
import re
import signal
import string
import subprocess as sp
import sys
import tempfile
import time
from typing import Any, Optional
import unittest

import numpy
from PIL import Image

import testlib.encoder as te
import testlib.image as tli

# Enable these to always write out, irrespective of test result
ASTCENC_CLI_ALWAYS = False
ASTCENC_LOG_ALWAYS = False

# Enable these to write out on failure for positive tests
ASTCENC_CLI_ON_ERROR = True
ASTCENC_LOG_ON_ERROR = True

# Enable these to write out on failure for negative tests
ASTCENC_CLI_ON_ERROR_NEG = True
ASTCENC_LOG_ON_ERROR_NEG = True

Color4Type = tuple[float, float, float, float]
ColorVType = tuple[float, ...]

# LDR test pattern
ASTCENC_TEST_PATTERN_LDR = {
    'TL': (0.00, 0.00, 0.00, 1.00),
    'TR': (1.00, 0.00, 0.00, 1.00),
    'BL': (1.00, 1.00, 1.00, 1.00),
    'BR': (0.25, 0.75, 0.00, 0.87)
}

# HDR test pattern
ASTCENC_TEST_PATTERN_HDR = {
    'TL': (0.00, 0.00, 0.00, 1.00),
    'TR': (8.00, 0.00, 0.00, 1.00),
    'BL': (3.98, 3.98, 3.98, 1.00),
    'BR': (0.25, 0.75, 0.00, 0.87)
}

LDR_RGB_PSNR_PATTERN = re.compile(r'\s*PSNR \(LDR-RGB\): (.*) dB')


class CLITestBase(unittest.TestCase):
    '''
    Command line interface base class.

    Attributes:
        binary: The path of the executable to test.
    '''
    test_encoder = 'avx2'

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        encoder = te.Encoder2x(self.test_encoder)
        self.binary = encoder.binary_path

    def setUp(self):
        '''
        Set up a test case.

        Create a new temporary directory for output files.
        '''
        # pylint: disable=consider-using-with
        self.tmp_dir = tempfile.TemporaryDirectory()

    def tearDown(self):
        '''
        Tear down a test case.

        Clean up the temporary directory created for output files.
        '''
        self.tmp_dir.cleanup()
        self.tmp_dir = None

    @staticmethod
    def get_image_path(image: str) -> Path:
        '''
        Get the path of a specific reference image on disk.

        Args:
            image: The image to load.

        Return:
            The path to the test image file on disk.
        '''
        script_dir = Path(__file__).parent
        return script_dir / 'Data' / image

    @staticmethod
    def get_ref_image_path(profile: str, mode: str, image: str) -> Path:
        '''
        Get the path of a reference image on disk.

        Args:
            profile: The color profile.
            mode: The type of image to load.
            image: The image variant to load.

        Return:
            The path to the test image file on disk.
        '''
        file_exts = {
            'LDR': {
                'input': 'png',
                'comp': 'astc'
            },
            'LDRS': {
                'input': 'png',
                'comp': 'astc'
            },
            'HDR': {
                'input': 'exr',
                'comp': 'astc'
            }
        }

        assert profile in file_exts
        assert mode in file_exts['LDR']

        script_dir = Path(__file__).parent
        file_name = f'{profile}-{image}-1x1.{file_exts[profile][mode]}'
        return script_dir / 'Data' / file_name

    def get_tmp_image_path(self, profile: str, mode: str) -> Path:
        '''
        Get the path of a temporary output image on disk.

        Temporary files are automatically cleaned up when the test tearDown
        occurs.

        Args:
            profile: The color profile. 'EXP' means explicit which means
                the 'mode' parameter is interpreted as a literal file
                extension not a symbolic mode.
            mode: The type of image to load.

        Return:
            The path to the test image file on disk.
        '''
        # Handle explicit mode
        if profile == 'EXP':
            file_handle, path = tempfile.mkstemp(mode, dir=self.tmp_dir.name)
            os.close(file_handle)
            os.remove(path)
            return Path(path)

        # Handle symbolic modes
        file_exts = {
            'LDR': {
                'comp': '.astc',
                'decomp': '.png',
                'bad': '.foo'
            },
            'LDRS': {
                'comp': '.astc',
                'decomp': '.png',
                'bad': '.foo'
            },
            'HDR': {
                'comp': '.astc',
                'decomp': '.exr',
                'bad': '.foo'
            }
        }

        assert profile in file_exts
        assert mode in file_exts['LDR']

        suffix = file_exts[profile][mode]
        file_handle, path = tempfile.mkstemp(suffix, dir=self.tmp_dir.name)
        os.close(file_handle)
        os.remove(path)
        return Path(path)


class CLIPTest(CLITestBase):
    '''
    Command line interface positive tests.
    '''
    # pylint: disable=too-many-public-methods

    def compare(self, image1: Path, image2: Path) -> bool:
        '''
        Utility function to compare two images.

        Note that this comparison tests only decoded color values; file
        metadata is ignored, and encoding methods are not compared.

        Args:
            image1: Path to the first image.
            image2: Path to the second image.

        Return:
            True if the images are the same, False otherwise.
        '''
        # Use Any here because PIL interface doesn't have clean types
        img1: Any = Image.open(image1)
        img2: Any = Image.open(image2)

        # Images must have same size
        if img1.size != img2.size:
            return False

        # Images must have same number of color channels
        if img1.getbands() != img2.getbands():
            # ... unless the only different is alpha
            self.assertEqual(img1.getbands(), ('R', 'G', 'B'))
            self.assertEqual(img2.getbands(), ('R', 'G', 'B', 'A'))

            # ... and the alpha is always one
            bands = img2.split()
            alpha_hist = bands[3].histogram()
            self.assertEqual(sum(alpha_hist[:-1]), 0)

            # Generate a version of img2 without alpha
            img2 = Image.merge('RGB', (bands[0], bands[1], bands[2]))

        # Compute sum of absolute differences
        dat1 = numpy.array(img1)
        dat2 = numpy.array(img2)
        sad = numpy.sum(numpy.abs(dat1 - dat2))
        return sad == 0

    def get_channel_rmse(self, image1: Path, image2: Path) -> list[float]:
        '''
        Get the channel-by-channel root mean square error.

        Images must have same channel count.

        Args:
            image1: Path to the first image.
            image2: Path to the second image.

        Return:
            List of floats containing the RMSE per channel.
        '''
        # Use Any here because PIL interface doesn't have clean types
        img1: Any = Image.open(image1)
        img2: Any = Image.open(image2)

        # Images must have same size
        assert img1.size == img2.size

        # Images must have same number of color channels
        if img1.getbands() != img2.getbands():
            # ... unless the only different is alpha
            self.assertEqual(img1.getbands(), ('R', 'G', 'B'))
            self.assertEqual(img2.getbands(), ('R', 'G', 'B', 'A'))

            # ... and the alpha is always one
            bands = img2.split()
            alpha_hist = bands[3].histogram()
            self.assertEqual(sum(alpha_hist[:-1]), 0)

            # Generate a version of img2 without alpha
            img2 = Image.merge('RGB', (bands[0], bands[1], bands[2]))

        # Compute root mean square error

        rmse_channels = []
        img_channels = zip(img1.split(), img2.split())
        for img1_channel, img2_channel in img_channels:
            texel_count = numpy.prod(img1_channel.size)
            diff = numpy.array(img1_channel) - numpy.array(img2_channel)
            sad = numpy.sum(numpy.square(diff))
            rmse = numpy.sqrt(numpy.divide(sad, texel_count))
            rmse_channels.append(rmse)

        return rmse_channels

    @staticmethod
    def get_color_refs(mode: str, corners: str) -> Color4Type:
        '''
        Get an expected reference color for a tile image corner.

        Args:
            mode: The color mode (LDR, or HDR)
            corners: The corner (TL, TR, BL, or BR)

        Return:
            The corner color value.
        '''
        modes = {
            'LDR': ASTCENC_TEST_PATTERN_LDR,
            'HDR': ASTCENC_TEST_PATTERN_HDR
        }

        return modes[mode][corners]

    def assertColorSame(self, color1: ColorVType, color2: ColorVType,
                        threshold: float = 0.02,
                        swizzle: Optional[str] = None) -> None:
        '''
        Test if a color is the similar to a reference.

        Will trigger a test failure if the colors are not within threshold.

        Args:
            color1: The reference color to compare with.
            color2: The new color to compare.
            threshold: The allowed deviation from reference (ratio).
            swizzle: The swizzle string (4 characters from the set rgba01), to
                be applied to the reference color.
        '''
        # pylint: disable=invalid-name, consider-using-generator

        # Two vectors must be same length to be comparable
        self.assertEqual(len(color1), len(color2))

        # Swizzle the reference color if needed
        if swizzle:
            self.assertEqual(len(swizzle), len(color1))

            # Static remaps for literal zero and 1
            remap_index = {
                '0': len(color1),
                '1': len(color1) + 1
            }

            # Dynamic remaps for channels that exist in reference
            if len(color1) >= 1:
                remap_index['r'] = 0
            if len(color1) >= 2:
                remap_index['g'] = 1
            if len(color1) >= 3:
                remap_index['b'] = 2
            if len(color1) >= 4:
                remap_index['a'] = 3

            remap_input = list(color1) + [0.0, 1.0]
            color1 = tuple([remap_input[remap_index[s]] for s in swizzle])

        for ref_channel, new_channel in zip(color1, color2):
            max_delta = ref_channel * threshold
            self.assertAlmostEqual(ref_channel, new_channel, delta=max_delta)

    def exec(self, cmd: list[str | Path],
             pattern: Optional[re.Pattern] = None) -> str:
        '''
        Execute a positive test.

        Will trigger a test failure if the subprocess return code is any value
        other than zero.

        Args:
            cmd: The cmd to execute.
            pattern: A regex pattern to search for, must contain a single group
                which will be returned to the caller. The test will fail if no
                pattern match is found.

        Return:
            The stdout output of the child process if no pattern, or the first
            group match from the pattern when applied to stdout output of the
            child process.
        '''
        try:
            cmd_str = [str(x) for x in cmd]
            result = sp.run(cmd_str, stdout=sp.PIPE, stderr=sp.PIPE,
                            text=True, check=True)
            rcode = result.returncode
            rstdout = result.stdout
            rstderr = result.stderr

        except sp.CalledProcessError as ex:
            rcode = ex.returncode
            rstdout = ex.stdout
            rstderr = ex.stderr

        passed = rcode == 0

        # Emit debug logging if needed
        if ASTCENC_CLI_ALWAYS or (not passed and ASTCENC_CLI_ON_ERROR):
            print('\n' + ' '.join(cmd_str))

        if ASTCENC_LOG_ALWAYS or (not passed and ASTCENC_LOG_ON_ERROR):
            print(rstdout)
            print(rstderr)

        if rcode < 0:
            msg = f'Exec died with signal {signal.Signals(-rcode).name}'
            self.assertGreaterEqual(rcode, 0, msg)

        if rcode > 0:
            msg = f'Exec died with application error {rcode}'
            self.assertEqual(rcode, 0, msg)

        # If there is a regex pattern provided, then search for it
        if pattern:
            if match := pattern.search(rstdout):
                return match.group(1)
            self.assertIsNotNone(match)

        return rstdout

    def test_ldr_compress(self) -> None:
        '''
        Test basic LDR compression.
        '''
        image_in = self.get_ref_image_path('LDR', 'input', 'A')
        image_out = self.get_tmp_image_path('LDR', 'comp')
        image_ref = self.get_ref_image_path('LDR', 'comp', 'A')

        cmd = [self.binary, '-cl', image_in, image_out, '6x6', '-exhaustive']
        self.exec(cmd)
        self.assertTrue(filecmp.cmp(image_ref, image_out, False))

    def test_srgb_compress(self) -> None:
        '''
        Test basic LDR sRGB compression.
        '''
        image_in = self.get_ref_image_path('LDRS', 'input', 'A')
        image_out = self.get_tmp_image_path('LDRS', 'comp')
        image_ref = self.get_ref_image_path('LDRS', 'comp', 'A')

        cmd = [self.binary, '-cs', image_in, image_out, '6x6', '-exhaustive']
        self.exec(cmd)
        self.assertTrue(filecmp.cmp(image_ref, image_out, False))

    def test_hdr_compress1(self) -> None:
        '''
        Test basic HDR + LDR alpha compression.
        '''
        image_in = self.get_ref_image_path('HDR', 'input', 'A')
        image_out = self.get_tmp_image_path('HDR', 'comp')
        image_ref = self.get_ref_image_path('HDR', 'comp', 'A')

        cmd = [self.binary, '-ch', image_in, image_out, '6x6', '-exhaustive']
        self.exec(cmd)
        self.assertTrue(filecmp.cmp(image_ref, image_out, False))

    def test_hdr_compress2(self) -> None:
        '''
        Test basic HDR + HDR alpha compression.
        '''
        image_in = self.get_ref_image_path('HDR', 'input', 'A')
        image_out = self.get_tmp_image_path('HDR', 'comp')
        image_ref = self.get_ref_image_path('HDR', 'comp', 'A')

        cmd = [self.binary, '-cH', image_in, image_out, '6x6', '-exhaustive']
        self.exec(cmd)
        self.assertTrue(filecmp.cmp(image_ref, image_out, False))

    def test_ldr_decompress(self) -> None:
        '''
        Test basic LDR decompression.
        '''
        image_in = self.get_ref_image_path('LDR', 'comp', 'A')
        image_out = self.get_tmp_image_path('LDR', 'decomp')
        image_ref = self.get_ref_image_path('LDR', 'input', 'A')

        cmd = [self.binary, '-dl', image_in, image_out]
        self.exec(cmd)
        self.assertTrue(self.compare(image_ref, image_out))

    def test_srgb_decompress(self) -> None:
        '''
        Test basic LDR sRGB decompression.
        '''
        image_in = self.get_ref_image_path('LDRS', 'comp', 'A')
        image_out = self.get_tmp_image_path('LDRS', 'decomp')
        image_ref = self.get_ref_image_path('LDRS', 'input', 'A')

        cmd = [self.binary, '-ds', image_in, image_out]
        self.exec(cmd)
        self.assertTrue(self.compare(image_ref, image_out))

    def test_hdr_decompress1(self) -> None:
        '''
        Test basic HDR + LDR alpha decompression.
        '''
        image_ref = self.get_ref_image_path('HDR', 'input', 'A')
        image_in = self.get_ref_image_path('HDR', 'comp', 'A')
        image_out = self.get_tmp_image_path('HDR', 'decomp')

        cmd = [self.binary, '-dh', image_in, image_out]
        self.exec(cmd)

        color_ref = tli.Image(image_ref).get_colors((0, 0))
        color_out = tli.Image(image_out).get_colors((0, 0))
        self.assertColorSame(color_ref, color_out)

    def test_hdr_decompress2(self) -> None:
        '''
        Test basic HDR + HDR alpha decompression.
        '''
        image_ref = self.get_ref_image_path('HDR', 'input', 'A')
        image_in = self.get_ref_image_path('HDR', 'comp', 'A')
        image_out = self.get_tmp_image_path('HDR', 'decomp')

        cmd = [self.binary, '-dH', image_in, image_out]
        self.exec(cmd)

        color_ref = tli.Image(image_ref).get_colors((0, 0))
        color_out = tli.Image(image_out).get_colors((0, 0))
        self.assertColorSame(color_ref, color_out)

    def test_ldr_roundtrip(self) -> None:
        '''
        Test basic LDR round-trip
        '''
        image_in = self.get_ref_image_path('LDR', 'input', 'A')
        image_out = self.get_tmp_image_path('LDR', 'decomp')

        cmd = [self.binary, '-tl', image_in, image_out, '6x6', '-exhaustive']
        self.exec(cmd)
        self.assertTrue(self.compare(image_in, image_out))

    def test_srgb_roundtrip(self) -> None:
        '''
        Test basic LDR sRGB round-trip
        '''
        image_in = self.get_ref_image_path('LDRS', 'input', 'A')
        image_out = self.get_tmp_image_path('LDRS', 'decomp')

        cmd = [self.binary, '-ts', image_in, image_out, '6x6', '-exhaustive']
        self.exec(cmd)
        self.assertTrue(self.compare(image_in, image_out))

    def test_hdr_roundtrip1(self) -> None:
        '''
        Test basic HDR + LDR alpha round-trip.
        '''
        image_in = self.get_ref_image_path('HDR', 'input', 'A')
        image_out = self.get_tmp_image_path('HDR', 'decomp')

        cmd = [self.binary, '-th', image_in, image_out, '6x6', '-exhaustive']
        self.exec(cmd)
        color_in = tli.Image(image_in).get_colors((0, 0))
        color_out = tli.Image(image_out).get_colors((0, 0))
        self.assertColorSame(color_in, color_out)

    def test_hdr_roundtrip2(self) -> None:
        '''
        Test basic HDR + HDR alpha round-trip.
        '''
        image_in = self.get_ref_image_path('HDR', 'input', 'A')
        image_out = self.get_tmp_image_path('HDR', 'decomp')

        cmd = [self.binary, '-tH', image_in, image_out, '6x6', '-exhaustive']
        self.exec(cmd)
        color_in = tli.Image(image_in).get_colors((0, 0))
        color_out = tli.Image(image_out).get_colors((0, 0))
        self.assertColorSame(color_in, color_out)

    def test_valid_2d_block_sizes(self) -> None:
        '''
        Test all valid block sizes are accepted (2D images).
        '''
        block_sizes = [
            '4x4', '5x4', '5x5', '6x5', '6x6', '8x5', '8x6',
            '10x5', '10x6', '8x8', '10x8', '10x10', '12x10', '12x12'
        ]

        image_in = self.get_ref_image_path('LDR', 'input', 'A')
        image_out = self.get_tmp_image_path('LDR', 'decomp')

        for block_size in block_sizes:
            with self.subTest(block_size=block_size):
                cmd = [
                    self.binary, '-tl',
                    image_in, image_out, block_size, '-exhaustive'
                ]

                self.exec(cmd)
                color_in = tli.Image(image_in).get_colors((0, 0))
                color_out = tli.Image(image_out).get_colors((0, 0))
                self.assertColorSame(color_in, color_out)

    def test_valid_3d_block_sizes(self) -> None:
        '''
        Test all valid block sizes are accepted (3D images).
        '''
        block_sizes = [
            '3x3x3',
            '4x3x3', '4x4x3', '4x4x4',
            '5x4x4', '5x5x4', '5x5x5',
            '6x5x5', '6x6x5', '6x6x6'
        ]

        image_in = self.get_ref_image_path('LDR', 'input', 'A')
        image_out = self.get_tmp_image_path('LDR', 'decomp')

        for block_size in block_sizes:
            with self.subTest(block_size=block_size):
                cmd = [
                    self.binary, '-tl',
                    image_in, image_out, block_size, '-exhaustive'
                ]

                self.exec(cmd)
                color_in = tli.Image(image_in).get_colors((0, 0))
                color_out = tli.Image(image_out).get_colors((0, 0))
                self.assertColorSame(color_in, color_out)

    def test_valid_presets(self) -> None:
        '''
        Test all valid presets are accepted
        '''
        presets = ['-fastest', '-fast', '-medium',
                   '-thorough', '-verythorough', '-exhaustive']

        image_in = self.get_ref_image_path('LDR', 'input', 'A')
        image_out = self.get_tmp_image_path('LDR', 'decomp')

        for preset in presets:
            with self.subTest(preset=preset):
                cmd = [self.binary, '-tl', image_in, image_out, '4x4', preset]
                self.exec(cmd)
                color_in = tli.Image(image_in).get_colors((0, 0))
                color_out = tli.Image(image_out).get_colors((0, 0))
                self.assertColorSame(color_in, color_out)

    def test_valid_ldr_input_formats(self) -> None:
        '''
        Test valid LDR input file formats.
        '''
        image_formats = ['bmp', 'dds', 'jpg', 'ktx', 'png', 'tga']

        for image_format in image_formats:
            with self.subTest(image_format=image_format):
                image_in = Path(f'./Test/Data/Tiles/ldr.{image_format}')
                image_out = self.get_tmp_image_path('LDR', 'decomp')

                cmd = [self.binary, '-tl', image_in, image_out, '4x4', '-fast']
                self.exec(cmd)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(image_format, 'ldr'):
                    color_in = tli.Image(image_in).get_colors((7, 7))
                    color_out = tli.Image(image_out).get_colors((7, 7))

                    # Catch exception and add fallback for tga handling
                    # having unstable origin in ImageMagick
                    try:
                        self.assertColorSame(color_in, color_out)
                        continue
                    except AssertionError as ex:
                        if image_format != 'tga':
                            raise ex

                    # Try yflipped TGA image
                    color_in = tli.Image(image_in).get_colors((7, 7))
                    color_out = tli.Image(image_out).get_colors((7, 1))
                    self.assertColorSame(color_in, color_out)

    def test_valid_uncomp_ldr_output_formats(self) -> None:
        '''
        Test valid uncompressed LDR output file formats.
        '''
        image_formats = ['bmp', 'dds', 'ktx', 'png', 'tga']

        for image_format in image_formats:
            with self.subTest(image_format=image_format):
                image_in = self.get_ref_image_path('LDR', 'input', 'A')
                image_out = self.get_tmp_image_path('EXP', f'.{image_format}')

                cmd = [self.binary, '-tl', image_in, image_out, '4x4', '-fast']
                self.exec(cmd)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(image_format, 'ldr'):
                    color_in = tli.Image(image_in).get_colors((7, 7))
                    color_out = tli.Image(image_out).get_colors((7, 7))
                    self.assertColorSame(color_in, color_out)

    def test_valid_comp_ldr_output_formats(self) -> None:
        '''
        Test valid compressed LDR output file formats.
        '''
        image_formats = ['astc', 'ktx']

        for image_format in image_formats:
            with self.subTest(image_format=image_format):
                image_in = self.get_ref_image_path('LDR', 'input', 'A')
                image_out = self.get_tmp_image_path('EXP', f'.{image_format}')
                image_out_dec = self.get_tmp_image_path('LDR', 'decomp')

                cmd = [self.binary, '-cl', image_in, image_out, '4x4', '-fast']
                self.exec(cmd)

                cmd = [self.binary, '-dl', image_out, image_out_dec]
                self.exec(cmd)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(image_format, 'ldr'):
                    color_in = tli.Image(image_in).get_colors((7, 7))
                    color_out = tli.Image(image_out_dec).get_colors((7, 7))
                    self.assertColorSame(color_in, color_out)

    def test_valid_hdr_input_formats(self) -> None:
        '''
        Test valid HDR input file formats.
        '''
        image_formats = ['exr', 'hdr']

        for image_format in image_formats:
            with self.subTest(image_format=image_format):
                image_in = Path(f'./Test/Data/Tiles/hdr.{image_format}')
                image_out = self.get_tmp_image_path('HDR', 'decomp')

                cmd = [self.binary, '-th', image_in, image_out, '4x4', '-fast']
                self.exec(cmd)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(image_format, 'hdr'):
                    color_in = tli.Image(image_in).get_colors((7, 7))
                    color_out = tli.Image(image_out).get_colors((7, 7))
                    self.assertColorSame(color_in, color_out)

    def test_valid_uncomp_hdr_output_formats(self) -> None:
        '''
        Test valid uncompressed HDR output file formats.
        '''
        image_formats = ['dds', 'exr', 'hdr', 'ktx']

        for image_format in image_formats:
            with self.subTest(image_format=image_format):
                image_in = self.get_ref_image_path('HDR', 'input', 'A')
                image_out = self.get_tmp_image_path('EXP', f'.{image_format}')

                cmd = [self.binary, '-th', image_in, image_out, '4x4', '-fast']
                self.exec(cmd)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(image_format, 'hdr'):
                    color_in = tli.Image(image_in).get_colors((7, 7))
                    color_out = tli.Image(image_out).get_colors((7, 7))
                    self.assertColorSame(color_in, color_out)

    def test_valid_comp_hdr_output_formats(self) -> None:
        '''
        Test valid compressed HDR output file formats.
        '''
        image_formats = ['astc', 'ktx']

        for image_format in image_formats:
            with self.subTest(image_format=image_format):
                image_in = self.get_ref_image_path('HDR', 'input', 'A')
                image_out = self.get_tmp_image_path('EXP', f'.{image_format}')
                image_out_dec = self.get_tmp_image_path('HDR', 'decomp')

                cmd = [self.binary, '-ch', image_in, image_out, '4x4', '-fast']
                self.exec(cmd)

                cmd = [self.binary, '-dh', image_out, image_out_dec]
                self.exec(cmd)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(image_format, 'hdr'):
                    color_in = tli.Image(image_in).get_colors((7, 7))
                    color_out = tli.Image(image_out_dec).get_colors((7, 7))
                    self.assertColorSame(color_in, color_out)

    def test_compress_normal_psnr(self) -> None:
        '''
        Test compression of normal textures using PSNR error metrics.
        '''
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        cmd = [
            self.binary, '-tl',
            './Test/Images/Small/LDR-XY/ldr-xy-00.png',
            image_dec, '5x5', '-exhaustive']

        ref_db = float(self.exec(cmd, LDR_RGB_PSNR_PATTERN))

        cmd.append('-normal')
        test_db = float(self.exec(cmd, LDR_RGB_PSNR_PATTERN))

        # Note that this test simply asserts that the '-normal_psnr' is
        # connected and affects the output. We don't test it does something
        # useful; that it outside the scope of this test case.
        self.assertNotEqual(ref_db, test_db)

    def test_compress_normal_percep(self) -> None:
        '''
        Test compression of normal textures using perceptual error metrics.
        '''
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        cmd = [
            self.binary, '-tl',
            './Test/Images/Small/LDR-XY/ldr-xy-00.png',
            image_dec, '4x4', '-exhaustive']

        ref_db = float(self.exec(cmd, LDR_RGB_PSNR_PATTERN))

        cmd.append('-normal')
        cmd.append('-perceptual')
        test_db = float(self.exec(cmd, LDR_RGB_PSNR_PATTERN))

        # Note that this test simply asserts that the '-normal -percep' is
        # connected and affects the output. We don't test it does something
        # useful; that it outside the scope of this test case.
        self.assertNotEqual(ref_db, test_db)

    def test_compress_esw(self) -> None:
        '''
        Test compression swizzles.
        '''
        # The swizzles to test
        swizzles = ['rgba', 'g0r1', 'rrrg']

        # Compress a swizzled image
        for swizzle in swizzles:
            with self.subTest(swizzle=swizzle):
                image_dec = self.get_tmp_image_path('LDR', 'decomp')

                cmd = [
                    self.binary, '-tl',
                    './Test/Data/Tiles/ldr.png',
                    image_dec, '4x4', '-exhaustive',
                    '-esw', swizzle]

                self.exec(cmd)

                # Fetch the three color
                img = tli.Image(image_dec)
                color_test = img.get_colors((7, 7))
                color_ref = self.get_color_refs('LDR', 'BR')
                self.assertColorSame(color_ref, color_test, swizzle=swizzle)

    def test_compress_dsw(self) -> None:
        '''
        Test decompression swizzles.
        '''
        # The swizzles to test
        swizzles = ['rgba', 'g0r1', 'rrrg']

        # Decompress a swizzled image
        for swizzle in swizzles:
            with self.subTest(swizzle=swizzle):
                image_dec = self.get_tmp_image_path('LDR', 'decomp')

                cmd = [
                    self.binary, '-tl',
                    './Test/Data/Tiles/ldr.png',
                    image_dec, '4x4', '-exhaustive',
                    '-dsw', swizzle]

                self.exec(cmd)

                # Fetch the three color
                img = tli.Image(image_dec)
                color_test = img.get_colors((7, 7))
                color_ref = self.get_color_refs('LDR', 'BR')
                self.assertColorSame(color_ref, color_test, swizzle=swizzle)

    def test_compress_esw_dsw(self) -> None:
        '''
        Test compression and decompression swizzles
        '''
        # Compress a swizzled image, and swizzle back in decompression
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        cmd = [
            self.binary, '-tl',
            './Test/Data/Tiles/ldr.png',
            image_dec, '4x4', '-exhaustive',
            '-esw', 'gbar', '-dsw', 'argb']

        self.exec(cmd)

        # Fetch the three color
        img = tli.Image(image_dec)
        color_test = img.get_colors((7, 7))
        color_ref = self.get_color_refs('LDR', 'BR')
        self.assertColorSame(color_ref, color_test)

    def test_compress_flip(self) -> None:
        '''
        Test LDR image flip on compression.
        '''
        # Compress a flipped image
        image_comp = self.get_tmp_image_path('LDR', 'comp')

        cmd = [
            self.binary, '-cl',
            './Test/Data/Tiles/ldr.png',
            image_comp, '4x4', '-fast', '-yflip']

        self.exec(cmd)

        # Decompress a non-flipped image
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        cmd = [
            self.binary, '-dl',
            image_comp,
            image_dec]

        self.exec(cmd)

        # Compare TL (0, 0) with BL - should match
        color_ref = self.get_color_refs('LDR', 'BL')

        img = tli.Image(image_dec)
        color_test = img.get_colors((0, 0))
        self.assertColorSame(color_ref, color_test)

    def test_decompress_flip(self) -> None:
        '''
        Test LDR image flip on decompression.
        '''
        # Compress a non-flipped image
        image_comp = self.get_tmp_image_path('LDR', 'comp')

        cmd = [
            self.binary, '-cl',
            './Test/Data/Tiles/ldr.png',
            image_comp, '4x4', '-fast']

        self.exec(cmd)

        # Decompress a flipped image
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        cmd = [
            self.binary, '-dl',
            image_comp,
            image_dec, '-yflip']

        self.exec(cmd)

        # Compare TL (0, 0) with BL - should match
        color_ref = self.get_color_refs('LDR', 'BL')

        img = tli.Image(image_dec)
        color_test = img.get_colors((0, 0))
        self.assertColorSame(color_ref, color_test)

    def test_roundtrip_flip(self) -> None:
        '''
        Test LDR image flip on roundtrip (no flip should occur).
        '''
        # Compress and decompressed a flipped LDR image
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        cmd = [
            self.binary, '-tl',
            './Test/Data/Tiles/ldr.png',
            image_dec, '4x4', '-fast', '-yflip']

        self.exec(cmd)

        # Compare TL (0, 0) with TL - should match - i.e. no flip
        color_ref = self.get_color_refs('LDR', 'TL')

        img = tli.Image(image_dec)
        color_test = img.get_colors((0, 0))

        self.assertColorSame(color_ref, color_test)

    def test_channel_weighting(self) -> None:
        '''
        Test channel weighting.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_base = self.get_channel_rmse(input_path, image_dec)

        # Note: Using -cw can result in a worse result than not using -cw,
        # with regressions in RMSE for the high-weighted channel. This is
        # particularly an issue in synthetic images, as they are more likely to
        # hit corner cases in the heuristics. It happens to 'work' for the
        # selected test image and these settings, but might start to fail in
        # future due to compressor changes.

        # Test each channel with a high weight
        for ch_idx, ch_name in ((0, 'R'), (1, 'G'), (2, 'B'), (3, 'A')):
            with self.subTest(channel=ch_name):
                cw_arg = [f'{(10 if x == ch_idx else 1)}' for x in range(0, 4)]
                cmd2 = cmd + ['-cw'] + cw_arg
                self.exec(cmd2)
                rmse_test = self.get_channel_rmse(input_path, image_dec)
                self.assertLess(rmse_test[ch_idx], rmse_base[ch_idx])

    def test_partition_count_limit(self) -> None:
        '''
        Test partition count limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-partitioncountlimit', '1']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_2partition_index_limit(self) -> None:
        '''
        Test partition index limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-2partitionindexlimit', '1']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_3partition_index_limit(self) -> None:
        '''
        Test partition index limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-3partitionindexlimit', '1']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_4partition_index_limit(self) -> None:
        '''
        Test partition index limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-4partitionindexlimit', '1']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_blockmode_limit(self) -> None:
        '''
        Test block mode limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-blockmodelimit', '25']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_refinement_limit(self) -> None:
        '''
        Test refinement limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-refinementlimit', '1']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_candidate_limit(self) -> None:
        '''
        Test candidate limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-candidatelimit', '1']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_db_cutoff_limit(self) -> None:
        '''
        Test db cutoff limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-dblimit', '10']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce cutoff quality
        self.assertGreater(rmse_test, rmse_ref)

    def test_2partition_early_limit(self) -> None:
        '''
        Test 2 partition early limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-2partitionlimitfactor', '1.0']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_3partition_early_limit(self) -> None:
        '''
        Test 3 partition early limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-3partitionlimitfactor', '1.0']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertNotEqual(rmse_test, rmse_ref)

    def test_2plane_correlation_limit(self) -> None:
        '''
        Test 2 plane correlation limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-2planelimitcorrelation', '0.1']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_2partition_candidate_limit(self) -> None:
        '''
        Test 2 partition partitioning candidate limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-2partitioncandidatelimit', '1']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_3partition_candidate_limit(self) -> None:
        '''
        Test 3 partition partitioning candidate limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-3partitioncandidatelimit', '1']
        self.exec(cmd)
        rmse_test = sum(self.get_channel_rmse(input_path, image_dec))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(rmse_test, rmse_ref)

    def test_4partition_candidate_limit(self) -> None:
        '''
        Test 4 partition partitioning candidate limit.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        self.exec(cmd)
        rmse_ref = sum(self.get_channel_rmse(input_path, image_dec))

        cmd += ['-4partitioncandidatelimit', '1']
        self.exec(cmd)

        # RMSE should get worse (higher) if we reduce search space
        # Don't check this here, as 4 partitions not used in any Small image
        # even for -exhaustive, BUT cmd line option must be accepted and
        # not error ...
        del rmse_ref
        # self.assertGreater(rmse_test, rmse_ref)

    @unittest.skipIf(os.cpu_count() == 1, 'Cannot test on single core host')
    def test_thread_count(self) -> None:
        '''
        Test codec thread count.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']

        start = time.time()
        self.exec(cmd)
        time_ref = time.time() - start

        cmd += ['-j', '1']
        start = time.time()
        self.exec(cmd)
        time_test = time.time() - start

        # Test time should get slower with fewer threads
        self.assertGreater(time_test, time_ref)

    def test_silent(self) -> None:
        '''
        Test silent
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the basic image without any channel weights
        cmd = [
            self.binary, '-tl',
            input_path, image_dec, '4x4', '-medium']
        stdout = self.exec(cmd)

        cmd += ['-silent']
        stdout_silent = self.exec(cmd)

        # Check that stdout is shorter in silent mode. Note that this doesn't
        # check that it is as silent as it should be, just that silent is wired
        # somewhere ...
        self.assertLess(len(stdout_silent), len(stdout))

    def test_image_quality_stability(self) -> None:
        '''
        Test that a round-trip and a file-based round-trip give same result.
        '''
        input_path = Path('./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png')
        image1_dec = self.get_tmp_image_path('LDR', 'decomp')
        image2_comp = self.get_tmp_image_path('LDR', 'comp')
        image2_dec = self.get_tmp_image_path('LDR', 'decomp')

        # Compute the first image using a direct round-trip
        cmd = [self.binary, '-tl', input_path, image1_dec, '4x4', '-medium']
        self.exec(cmd)

        # Compute the first image using a file-based round-trip
        cmd = [
            self.binary, '-cl',
            input_path, image2_comp, '4x4', '-medium',
            '-decode_unorm8'
        ]

        self.exec(cmd)

        cmd = [
            self.binary, '-dl',
            image2_comp, image2_dec
        ]

        self.exec(cmd)

        # RMSE should be the same
        image1_rmse: float = sum(self.get_channel_rmse(input_path, image1_dec))
        image2_rmse: float = sum(self.get_channel_rmse(input_path, image2_dec))
        self.assertEqual(image1_rmse, image2_rmse)


class CLINTest(CLITestBase):
    '''
    Command line interface negative tests.

    These tests are designed to test that bad inputs to the cmd line are
    handled cleanly and that errors are correctly thrown.

    Note that many tests are mutations of a valid positive test cmd line,
    to ensure that the base cmd line is valid before it is mutated many
    of these tests include a *positive test* to ensure that the starting point
    is actually a valid cmd line (otherwise we could be throwing an
    arbitrary error).
    '''
    # pylint: disable=too-many-public-methods

    def exec(self, cmd: list[str | Path], expect_pass: bool = False) -> None:
        '''
        Execute a negative test.

        Test will automatically fail if:

        * The subprocess return code is zero, unless expect_pass is True.
        * The subprocess correctly returned non-zero, but without any error
          message.
        * The subprocess dies with any kind of signal.

        Args:
            cmd (list(str)): The cmd to execute.
            expect_pass (bool): True if this cmd is actually expected to
                pass, which is used to validate commands before mutating them.
        '''
        try:
            cmd_str = [str(x) for x in cmd]
            result = sp.run(cmd_str, stdout=sp.PIPE, stderr=sp.PIPE,
                            text=True, check=True)
            rcode = result.returncode
            rstdout = result.stdout
            rstderr = result.stderr

        except sp.CalledProcessError as ex:
            rcode = ex.returncode
            rstdout = ex.stdout
            rstderr = ex.stderr

        passed = rcode == 0

        # Emit debug logging if needed (negative rcode is a signal)
        bad_result = (passed != expect_pass) or (rcode < 0)

        if ASTCENC_CLI_ALWAYS or (bad_result and ASTCENC_CLI_ON_ERROR_NEG):
            print('\n' + ' '.join(cmd_str))

        if ASTCENC_LOG_ALWAYS or (bad_result and ASTCENC_LOG_ON_ERROR_NEG):
            print(rstdout)
            print(rstderr)

        # If we expected a pass, then rcode == 0
        if expect_pass:
            self.assertEqual(rcode, 0, 'Exec did not pass as expected')
            self.assertNotIn('ERROR', rstdout)
            self.assertNotIn('ERROR', rstderr)
            return

        # If we got a negative that's always bad (signal of some kind)
        if rcode < 0:
            msg = f'Exec died with signal {signal.Signals(-rcode).name}'
            self.assertGreaterEqual(rcode, 0, msg)

        # Otherwise just assert that we got an error log, and some positive
        # return code value was returned
        self.assertIn('ERROR', rstderr)
        self.assertGreater(rcode, 0, 'Exec did not fail as expected')

    def exec_with_omit(self, cmd: list[str | Path], omit_from: int) -> None:
        '''
        Execute a negative test with cmd line argument omission.

        These tests aim to prove that the cmd fails if arguments are
        missing. However the passed cmd MUST be a valid cmd which
        passes if no argument are omitted (this is checked, to ensure that
        the test case is a valid test).

        Test will automatically fail if:

        * A partial cmd doesn't fail.
        * The full cmd doesn't pass.

        Args:
            cmd: The command to run.
            omit_from: The index at which to start dropping arguments.
        '''
        # Run the cmd, incrementally omitting arguments
        cmd_len = len(cmd)
        for omit_index in range(omit_from, cmd_len + 1):
            omit = cmd_len - omit_index
            with self.subTest(omit=omit):
                cmd_modified = cmd[:omit_index]
                expect_pass = omit == 0
                self.exec(cmd_modified, expect_pass)

    def test_cl_missing_args(self) -> None:
        '''
        Test -cl with missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast']

        self.exec_with_omit(cmd, 2)

    def test_cl_missing_input(self) -> None:
        '''
        Test -cl with a missing input file.
        '''
        # Build a valid cmd with a missing input file
        cmd = [
            self.binary, '-cl',
            './Test/Data/missing.png',
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast']

        self.exec(cmd)

    def test_cl_missing_input_array_slice(self) -> None:
        '''
        Test -cl with a missing input file in an array slice.
        '''
        # Build a valid cmd with a missing input file
        cmd = [
            self.binary, '-cl',
            './Test/Data/Tiles/ldr.png',
            self.get_tmp_image_path('LDR', 'comp'),
            '3x3x3', '-fast', '-zdim', '3']

        self.exec(cmd)

    def test_cl_unknown_input(self) -> None:
        '''
        Test -cl with an unknown input file extension.
        '''
        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-cl',
            './Test/Data/empty.unk',
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast']

        self.exec(cmd)

    def test_cl_missing_output(self) -> None:
        '''
        Test -cl with a missing output directory.
        '''
        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            './DoesNotExist/test.astc',
            '4x4', '-fast']

        self.exec(cmd)

    def test_cl_unknown_output(self) -> None:
        '''
        Test -cl with an unknown output file extension.
        '''
        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            './test.aastc',
            '4x4', '-fast']

        self.exec(cmd)

    def test_cl_bad_block_size(self) -> None:
        '''
        Test -cl with an invalid block size.
        '''
        bad_sizes = [
            '4x5',      # Illegal 2D block size
            '3x3x4',    # Illegal 3D block size
            '4x4x4x4',  # Too many dimensions
            '4x',       # Incomplete 2D block size
            '4x4x',     # Incomplete 3D block size
            '4x4x4x',   # Over-long 3D block size
            '4xe',      # Illegal non-numeric character
            '4x4e'      # Additional non-numeric character
        ]

        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast']

        # Test that the underlying cmd is valid
        self.exec(cmd, True)

        block_index = cmd.index('4x4')
        for bad_size in bad_sizes:
            with self.subTest(block_size=bad_size):
                cmd[block_index] = bad_size
                self.exec(cmd)

    def test_cl_bad_preset(self) -> None:
        '''
        Test -cl with an invalid encoding preset.
        '''
        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fastt']

        self.exec(cmd)

    def test_cl_bad_argument(self) -> None:
        '''
        Test -cl with an unknown additional argument.
        '''
        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast', '-unknown']

        self.exec(cmd)

    def test_cl_2d_block_with_array(self) -> None:
        '''
        Test -cl with a 2D block size and 3D input data.
        '''
        # Build an otherwise valid cmd with the test flaw

        cmd = [
            self.binary, '-cl',
            './Test/Data/Tiles/ldr.png',
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast', '-zdim', '2']

        self.exec(cmd)

    def test_cl_array_missing_args(self) -> None:
        '''
        Test -cl with a 2D block size and 3D input data.
        '''
        # Build an otherwise valid cmd
        cmd = [
            self.binary, '-cl',
            './Test/Data/Tiles/ldr.png',
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4x4', '-fast', '-zdim', '2']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_tl_missing_args(self) -> None:
        '''
        Test -tl with missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-tl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'decomp'),
            '4x4', '-fast']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 2)

    def test_tl_missing_input(self) -> None:
        '''
        Test -tl with a missing input file.
        '''
        # Build a valid cmd with a missing input file
        cmd = [
            self.binary, '-tl',
            './Test/Data/missing.png',
            self.get_tmp_image_path('LDR', 'decomp'),
            '4x4', '-fast']

        self.exec(cmd)

    def test_tl_unknown_input(self) -> None:
        '''
        Test -tl with an unknown input file extension.
        '''
        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-tl',
            './Test/Data/empty.unk',
            self.get_tmp_image_path('LDR', 'decomp'),
            '4x4', '-fast']

        self.exec(cmd)

    def test_tl_missing_output(self) -> None:
        '''
        Test -tl with a missing output directory.
        '''
        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-tl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            './DoesNotExist/test.png',
            '4x4', '-fast']

        self.exec(cmd)

    def test_tl_bad_block_size(self) -> None:
        '''
        Test -tl with an invalid block size.
        '''
        bad_sizes = [
            '4x5',      # Illegal 2D block size
            '3x3x4',    # Illegal 3D block size
            '4x4x4x4',  # Too many dimensions
            '4x',       # Incomplete 2D block size
            '4x4x',     # Incomplete 3D block size
            '4x4x4x',   # Over-long 3D block size
            '4xe',      # Illegal non-numeric character
            '4x4e'      # Additional non-numeric character
        ]

        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-tl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'decomp'),
            '4x4', '-fast']

        # Test that the underlying cmd is valid
        self.exec(cmd, True)

        block_index = cmd.index('4x4')
        for bad_size in bad_sizes:
            with self.subTest(block_size=bad_size):
                cmd[block_index] = bad_size
                self.exec(cmd)

    def test_tl_bad_preset(self) -> None:
        '''
        Test -tl with an invalid encoding preset.
        '''
        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-tl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'decomp'),
            '4x4', '-fastt']

        self.exec(cmd)

    def test_tl_bad_argument(self) -> None:
        '''
        Test -tl with an unknown additional argument.
        '''
        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-tl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'decomp'),
            '4x4', '-fast', '-unknown']

        self.exec(cmd)

    def test_dl_missing_args(self) -> None:
        '''
        Test -dl with missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-dl',
            self.get_ref_image_path('LDR', 'comp', 'A'),
            self.get_tmp_image_path('LDR', 'decomp')]

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 2)

    def test_dl_missing_output(self) -> None:
        '''
        Test -dl with a missing output directory.
        '''
        # Build an otherwise valid cmd with the test flaw
        cmd = [
            self.binary, '-dl',
            self.get_ref_image_path('LDR', 'comp', 'A'),
            './DoesNotExist/test.png']

        self.exec(cmd)

    def test_cl_a_missing_args(self) -> None:
        '''
        Test -cl with -a and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-a', '2']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_cw_missing_args(self) -> None:
        '''
        Test -cl with -cw and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-cw', '0', '1', '2', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_2partitionindexlimit_missing_args(self) -> None:
        '''
        Test -cl with -2partitionindexlimit and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-2partitionindexlimit', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_3partitionindexlimit_missing_args(self) -> None:
        '''
        Test -cl with -3partitionindexlimit and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-3partitionindexlimit', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_4partitionindexlimit_missing_args(self) -> None:
        '''
        Test -cl with -4partitionindexlimit and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-4partitionindexlimit', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_2partitioncandidatelimit_missing_args(self) -> None:
        '''
        Test -cl with -2partitioncandidatelimit and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-2partitioncandidatelimit', '1']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_3partitioncandidatelimit_missing_args(self) -> None:
        '''
        Test -cl with -3partitioncandidatelimit and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-3partitioncandidatelimit', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_4partitioncandidatelimit_missing_args(self) -> None:
        '''
        Test -cl with -4partitioncandidatelimit and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-4partitioncandidatelimit', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_blockmodelimit_missing_args(self) -> None:
        '''
        Test -cl with -blockmodelimit and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-blockmodelimit', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_refinementlimit_missing_args(self) -> None:
        '''
        Test -cl with -refinementlimit and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-refinementlimit', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_dblimit_missing_args(self) -> None:
        '''
        Test -cl with -dblimit and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-dblimit', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_2partitionearlylimit_missing_args(self) -> None:
        '''
        Test -cl with -2partitionlimitfactor and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-2partitionlimitfactor', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_3partitionearlylimit_missing_args(self) -> None:
        '''
        Test -cl with -3partitionlimitfactor and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-3partitionlimitfactor', '3']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_2planeearlylimit_missing_args(self) -> None:
        '''
        Test -cl with -2planelimitcorrelation and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-2planelimitcorrelation', '0.66']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_esw_missing_args(self) -> None:
        '''
        Test -cl with -esw and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-esw', 'rgb1']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_esw_invalid_swizzle(self) -> None:
        '''
        Test -cl with -esw and invalid swizzles.
        '''
        bad_swizzles = [
            '',  # Short swizzles
            'r',
            'rr',
            'rrr',
            'rrrrr',  # Long swizzles
        ]

        # Create swizzles with all invalid printable ascii codes
        good = ['r', 'g', 'b', 'a', '0', '1']
        for channel in string.printable:
            if channel not in good:
                bad_swizzles.append(channel * 4)

        # Build a valid base cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-esw', 'rgba']

        block_index = cmd.index('rgba')
        for bad_swizzle in bad_swizzles:
            with self.subTest(swizzle=bad_swizzle):
                cmd[block_index] = bad_swizzle
                self.exec(cmd)

    def test_cl_ssw_missing_args(self) -> None:
        '''
        Test -cl with -ssw and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-ssw', 'rgba']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_cl_ssw_invalid_swizzle(self) -> None:
        '''
        Test -cl with -ssw and invalid swizzles.
        '''
        bad_swizzles = [
            '',  # Short swizzles
            'rrrrr',  # Long swizzles
        ]

        # Create swizzles with all invalid printable ascii codes
        good = ['r', 'g', 'b', 'a']
        for channel in string.printable:
            if channel not in good:
                bad_swizzles.append(channel * 4)

        # Build a valid base cmd
        cmd = [
            self.binary, '-cl',
            self.get_ref_image_path('LDR', 'input', 'A'),
            self.get_tmp_image_path('LDR', 'comp'),
            '4x4', '-fast',
            '-ssw', 'rgba']

        block_index = cmd.index('rgba')
        for bad_swizzle in bad_swizzles:
            with self.subTest(swizzle=bad_swizzle):
                cmd[block_index] = bad_swizzle
                self.exec(cmd)

    def test_dl_dsw_missing_args(self) -> None:
        '''
        Test -dl with -dsw and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-dl',
            self.get_ref_image_path('LDR', 'comp', 'A'),
            self.get_tmp_image_path('LDR', 'decomp'),
            '-dsw', 'rgb1']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 5)

    def test_dl_dsw_invalid_swizzle(self) -> None:
        '''
        Test -dl with -dsw and invalid swizzles.
        '''
        bad_swizzles = [
            '',  # Short swizzles
            'r',
            'rr',
            'rrr',
            'rrrrr',  # Long swizzles
        ]

        # Create swizzles with all invalid printable ascii codes
        good = ['r', 'g', 'b', 'a', 'z', '0', '1']
        for channel in string.printable:
            if channel not in good:
                bad_swizzles.append(channel * 4)

        # Build a valid base cmd
        cmd = [
            self.binary, '-dl',
            self.get_ref_image_path('LDR', 'comp', 'A'),
            self.get_tmp_image_path('LDR', 'decomp'),
            '-dsw', 'rgba']

        block_index = cmd.index('rgba')
        for bad_swizzle in bad_swizzles:
            with self.subTest(swizzle=bad_swizzle):
                cmd[block_index] = bad_swizzle
                self.exec(cmd)

    def test_ch_mpsnr_missing_args(self) -> None:
        '''
        Test -ch with -mpsnr and missing arguments.
        '''
        # Build a valid cmd
        cmd = [
            self.binary, '-ch',
            self.get_ref_image_path('HDR', 'input', 'A'),
            self.get_tmp_image_path('HDR', 'comp'),
            '4x4', '-fast',
            '-mpsnr', '-5', '5']

        # Run the cmd, incrementally omitting arguments
        self.exec_with_omit(cmd, 7)

    def test_dl_corrupt_astc_magic(self) -> None:
        '''
        Test -dl with an astc file with the wrong magic number.
        '''
        # Build a valid cmd with a bad image file
        cmd = [
            self.binary, '-dl',
            self.get_image_path('negative_magic.astc'),
            self.get_tmp_image_path('LDR', 'decomp')]

        self.exec(cmd)

    def test_dl_corrupt_astc_huge_size(self) -> None:
        '''
        Test -dl with an astc file with a data size that will fail alloc.

        Note this test will cause ASAN to error because ASAN itself cannot cope
        with OOM issues. ASAN documentation says you can run with environment
        ASAN_OPTIONS=allocator_may_return_null=1, but this doesn't seem to work
        for allocations made with C++ new[]. This test will need to be disabled
        when ASAN is used.
        '''
        # Build a valid cmd with a bad image file
        cmd = [
            self.binary, '-dl',
            self.get_image_path('negative_huge.astc'),
            self.get_tmp_image_path('LDR', 'decomp')]

        self.exec(cmd)

    def test_dl_corrupt_astc_overflow_size(self) -> None:
        '''
        Test -dl with an astc file with a total data size that overflows.
        '''
        # Build a valid cmd with a bad image file
        cmd = [
            self.binary, '-dl',
            self.get_image_path('negative_overflow.astc'),
            self.get_tmp_image_path('LDR', 'decomp')]

        self.exec(cmd)

    def test_dl_corrupt_astc_short_data(self) -> None:
        '''
        Test -dl with an astc file missing data.
        '''
        # Build a valid cmd with a bad image file
        cmd = [
            self.binary, '-dl',
            self.get_image_path('negative_short.astc'),
            self.get_tmp_image_path('LDR', 'decomp')]

        self.exec(cmd)

    def test_dl_corrupt_astc_bad_block_size(self) -> None:
        '''
        Test -dl with an astc file with invalid block size.
        '''
        # Build a valid cmd with a bad image file
        cmd = [
            self.binary, '-dl',
            self.get_image_path('negative_block_size.astc'),
            self.get_tmp_image_path('LDR', 'decomp')]

        self.exec(cmd)


def main() -> int:
    '''
    The main function.

    Return:
        The process return code.
    '''
    parser = argparse.ArgumentParser()

    coders = ['none', 'neon', 'sve_128', 'sve_256', 'sse2', 'sse4.1', 'avx2']
    parser.add_argument('--encoder', dest='encoder', default='avx2',
                        choices=coders, help='test encoder variant')
    args = parser.parse_known_args()

    # Set the encoder for this test run
    CLITestBase.test_encoder = args[0].encoder

    # Set the sys.argv to remaining args (leave sys.argv[0] alone)
    sys.argv[1:] = args[1]

    results = unittest.main(exit=False)
    return 0 if results.result.wasSuccessful() else 1


if __name__ == '__main__':
    sys.exit(main())
