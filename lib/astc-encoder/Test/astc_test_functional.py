#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2020-2023 Arm Limited
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
The functional test runner is a set of tests that validate the ``astcenc``
command line is correctly handled, under both valid and invalid usage
scenarios. These tests do NOT validate the compression codec itself, beyond
some very basic incidental usage needed to validate the command line.

Due to the need to validate pixel colors in test images for both LDR and HDR
images, these tests rely on an HDRI-enabled build of ImageMagic being available
on the system path. To test if the version of ImageMagic on your system is
HDRI-enabled run:

    convert --version

... and check that the string "HDRI" is present in the listed features.

Test Tiles
==========

Some basic test images, each 8x8 texels and built up from 4 no. 4x4 texel
constant color blocks, are used to help determine that the command line is
being processed correctly.

LDR Test Pattern
----------------

LDR images are an 8x8 image containing 4 4x4 constant color blocks. Assuming
(0, 0) is the top left (TL), the component uncompressed block colors are:

* (0, 0) TL = Black, opaque = (0.00, 0.00, 0.00, 1.00)
* (7, 0) TR = Red, opaque   = (1.00, 0.00, 0.00, 1.00)
* (0, 7) BL = White, opaque = (1.00, 1.00, 1.00, 1.00)
* (7, 7) BR = Green, trans  = (0.25, 0.75, 0.00, 0.87)

HDR Test Pattern
----------------

HDR images are an 8x8 image containing 4 4x4 constant color blocks. Assuming
(0, 0) is the top left (TL), the component uncompressed block colors are:

* (0, 0) TL = LDR Black, opaque = (0.00, 0.00, 0.00, 1.00)
* (7, 0) TR = HDR Red, opaque   = (8.00, 0.00, 0.00, 1.00)
* (0, 7) BL = HDR White, opaque = (3.98, 3.98, 3.98, 1.00)
* (7, 7) BR = LDR Green, trans  = (0.25, 0.75, 0.00, 0.87)
"""

import argparse
import filecmp
import os
import re
import signal
import string
import subprocess as sp
import sys
import tempfile
import time
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

# LDR test pattern
ASTCENC_TEST_PATTERN_LDR = {
    "TL": (0.00, 0.00, 0.00, 1.00),
    "TR": (1.00, 0.00, 0.00, 1.00),
    "BL": (1.00, 1.00, 1.00, 1.00),
    "BR": (0.25, 0.75, 0.00, 0.87)
}

# HDR test pattern
ASTCENC_TEST_PATTERN_HDR = {
    "TL": (0.00, 0.00, 0.00, 1.00),
    "TR": (8.00, 0.00, 0.00, 1.00),
    "BL": (3.98, 3.98, 3.98, 1.00),
    "BR": (0.25, 0.75, 0.00, 0.87)
}

LDR_RGB_PSNR_PATTERN = re.compile(r"\s*PSNR \(LDR-RGB\): (.*) dB")

g_TestEncoder = "avx2"

class CLITestBase(unittest.TestCase):
    """
    Command line interface base class.

    These tests are designed to test the command line is handled correctly.
    They are not detailed tests of the codec itself; only basic sanity checks
    that some type of processing occurred are used.
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        encoder = te.Encoder2x(g_TestEncoder)
        self.binary = encoder.binary

    def setUp(self):
        """
        Set up a test case.

        Create a new temporary directory for output files.
        """
        self.tempDir = tempfile.TemporaryDirectory()

    def tearDown(self):
        """
        Tear down a test case.

        Clean up the temporary directory created for output files.
        """
        self.tempDir.cleanup()
        self.tempDir = None

    @staticmethod
    def get_ref_image_path(profile, mode, image):
        """
        Get the path of a reference image on disk.

        Args:
            profile (str): The color profile.
            mode (str): The type of image to load.
            image (str): The image variant to load.

        Returns:
            str: The path to the test image file on disk.
        """
        nameMux = {
            "LDR": {
                "input": "png",
                "comp": "astc"
            },
            "LDRS": {
                "input": "png",
                "comp": "astc"
            },
            "HDR": {
                "input": "exr",
                "comp": "astc"
            }
        }

        assert profile in nameMux.keys()
        assert mode in nameMux["LDR"].keys()

        scriptDir = os.path.dirname(__file__)
        fileName = "%s-%s-1x1.%s" % (profile, image, nameMux[profile][mode])
        return os.path.join(scriptDir, "Data", fileName)

    def get_tmp_image_path(self, profile, mode):
        """
        Get the path of a temporary output image on disk.

        Temporary files are automatically cleaned up when the test tearDown
        occurs.

        Args:
            profile (str): The color profile. "EXP" means explicit which means
                the "mode" parameter is interpreted as a literal file
                extension not a symbolic mode.
            mode (str): The type of image to load.

        Returns:
            str: The path to the test image file on disk.
        """
        # Handle explicit mode
        if profile == "EXP":
            tmpFile, tmpPath = tempfile.mkstemp(mode, dir=self.tempDir.name)
            os.close(tmpFile)
            os.remove(tmpPath)
            return tmpPath

        # Handle symbolic modes
        nameMux = {
            "LDR": {
                "comp": ".astc",
                "decomp": ".png",
                "bad": ".foo"
            },
            "LDRS": {
                "comp": ".astc",
                "decomp": ".png",
                "bad": ".foo"
            },
            "HDR": {
                "comp": ".astc",
                "decomp": ".exr",
                "bad": ".foo"
            }
        }

        assert profile in nameMux.keys()
        assert mode in nameMux["LDR"].keys()

        suffix = nameMux[profile][mode]
        tmpFile, tmpPath = tempfile.mkstemp(suffix, dir=self.tempDir.name)
        os.close(tmpFile)
        os.remove(tmpPath)
        return tmpPath


class CLIPTest(CLITestBase):
    """
    Command line interface positive tests.

    These tests are designed to test the command line is handled correctly.
    They are not detailed tests of the codec itself; only basic sanity checks
    that some type of processing occurred are used.
    """

    def compare(self, image1, image2):
        """
        Utility function to compare two images.

        Note that this comparison tests only decoded color values; any file
        metadata is ignored, and encoding methods are not compared.

        Args:
            image1 (str): Path to the first image.
            image2 (str): Path to the second image.

        Returns:
            bool: ``True` if the images are the same, ``False`` otherwise.
        """
        img1 = Image.open(image1)
        img2 = Image.open(image2)

        # Images must have same size
        if img1.size != img2.size:
            print("Size")
            return False

        # Images must have same number of color channels
        if img1.getbands() != img2.getbands():
            # ... unless the only different is alpha
            self.assertEqual(img1.getbands(), ("R", "G", "B"))
            self.assertEqual(img2.getbands(), ("R", "G", "B", "A"))

            # ... and the alpha is always one
            bands = img2.split()
            alphaHist = bands[3].histogram()
            self.assertEqual(sum(alphaHist[:-1]), 0)

            # Generate a version of img2 without alpha
            img2 = Image.merge("RGB", (bands[0], bands[1], bands[2]))

        # Compute sum of absolute differences
        dat1 = numpy.array(img1)
        dat2 = numpy.array(img2)
        sad = numpy.sum(numpy.abs(dat1 - dat2))

        if sad != 0:
            print(img1.load()[0, 0])
            print(img2.load()[0, 0])

        return sad == 0

    def get_channel_rmse(self, image1, image2):
        """
        Get the channel-by-channel root mean square error.

        Args:
            image1 (str): Path to the first image.
            image2 (str): Path to the second image.

        Returns:
            tuple: Tuple of floats containing the RMSE per channel.
            None: Images could not be compared because they are different size.
        """
        img1 = Image.open(image1)
        img2 = Image.open(image2)

        # Images must have same size
        if img1.size != img2.size:
            return None

        # Images must have same number of color channels
        if img1.getbands() != img2.getbands():
            # ... unless the only different is alpha
            self.assertEqual(img1.getbands(), ("R", "G", "B"))
            self.assertEqual(img2.getbands(), ("R", "G", "B", "A"))

            # ... and the alpha is always one
            bands = img2.split()
            alphaHist = bands[3].histogram()
            self.assertEqual(sum(alphaHist[:-1]), 0)

            # Generate a version of img2 without alpha
            img2 = Image.merge("RGB", (bands[0], bands[1], bands[2]))

        # Compute root mean square error
        img1bands = img1.split()
        img2bands = img2.split()

        rmseVals = []
        imgBands = zip(img1bands, img2bands)
        for img1Ch, img2Ch in imgBands:
            imSz = numpy.prod(img1Ch.size)
            dat1 = numpy.array(img1Ch)
            dat2 = numpy.array(img2Ch)

            sad = numpy.sum(numpy.square(dat1 - dat2))
            mse = numpy.divide(sad, imSz)
            rmse = numpy.sqrt(mse)
            rmseVals.append(rmse)

        return rmseVals

    @staticmethod
    def get_color_refs(mode, corners):
        """
        Build a set of reference colors from apriori color list.

        Args:
            mode (str): The color mode (LDR, or HDR)
            corners (str or list): The corner or list of corners -- named TL,
                TR, BL, and BR -- to return.

        Returns:
            tuple: The color value, if corners was a name.
            [tuple]: List of color values, if corners was a list of names.
        """
        modes = {
            "LDR": ASTCENC_TEST_PATTERN_LDR,
            "HDR": ASTCENC_TEST_PATTERN_HDR
        }

        if isinstance(corners, str):
            return [modes[mode][corners]]

        return [modes[mode][corner] for corner in corners]

    def assertColorSame(self, colorRef, colorNew, threshold=0.02, swiz=None):
        """
        Test if a color is the similar to a reference.

        Will trigger a test failure if the colors are not within threshold.

        Args:
            colorRef (tuple): The reference color to compare with.
            colorNew (tuple): The new color.
            threshold (float): The allowed deviation from colorRef (ratio).
            swiz (str): The swizzle string (4 characters from the set
                `rgba01`), applied to the reference color.
        """
        self.assertEqual(len(colorRef), len(colorNew))

        # Swizzle the reference color if needed
        if swiz:
            self.assertEqual(len(swiz), len(colorRef))

            remap = {
                "0": len(colorRef),
                "1": len(colorRef) + 1
            }

            if len(colorRef) >= 1:
                remap["r"] = 0
            if len(colorRef) >= 2:
                remap["g"] = 1
            if len(colorRef) >= 3:
                remap["b"] = 2
            if len(colorRef) >= 4:
                remap["a"] = 3

            colorRefExt = list(colorRef) + [0.0, 1.0]
            colorRef = [colorRefExt[remap[s]] for s in swiz]

        for chRef, chNew in zip(colorRef, colorNew):
            deltaMax = chRef * threshold
            self.assertAlmostEqual(chRef, chNew, delta=deltaMax)

    def exec(self, command, pattern=None):
        """
        Execute a positive test.

        Will trigger a test failure if the subprocess return code is any value
        other than zero.

        Args:
            command (list(str)): The command to execute.
            pattern (re.Pattern): The regex pattern to search for, must
                contain a single group (this is returned to the caller). The
                test will fail if no pattern match is found.

        Returns:
            str: The stdout output of the child process, or the first group
               from the passed regex pattern.
        """
        try:
            result = sp.run(command, stdout=sp.PIPE, stderr=sp.PIPE,
                            universal_newlines=True, check=True)
            error = False
        except sp.CalledProcessError as ex:
            result = ex
            error = True

        # Emit debug logging if needed
        if ASTCENC_CLI_ALWAYS or (error and ASTCENC_CLI_ON_ERROR):
            # Format for shell replay
            print("\n" + " ".join(command))
            # Format for script command list replay
            print("\n" + ", ".join(("\"%s\"" % x for x in command)))

        if ASTCENC_LOG_ALWAYS or (error and ASTCENC_LOG_ON_ERROR):
            print(result.stdout)

        rcode = result.returncode

        if rcode < 0:
            msg = "Exec died with signal %s" % signal.Signals(-rcode).name
            self.assertGreaterEqual(rcode, 0, msg)

        if rcode > 0:
            msg = "Exec died with application error %u" % rcode
            self.assertEqual(rcode, 0, msg)

        # If there is a regex pattern provided, then search for it
        if pattern:
            match = pattern.search(result.stdout)
            self.assertIsNotNone(match)
            return match.group(1)

        return result.stdout

    def test_ldr_compress(self):
        """
        Test basic LDR compression.
        """
        imIn = self.get_ref_image_path("LDR", "input", "A")
        imOut = self.get_tmp_image_path("LDR", "comp")
        imRef = self.get_ref_image_path("LDR", "comp", "A")

        command = [self.binary, "-cl", imIn, imOut, "6x6", "-exhaustive"]
        self.exec(command)
        self.assertTrue(filecmp.cmp(imRef, imOut, False))

    def test_srgb_compress(self):
        """
        Test basic LDR sRGB compression.
        """
        imIn = self.get_ref_image_path("LDRS", "input", "A")
        imOut = self.get_tmp_image_path("LDRS", "comp")
        imRef = self.get_ref_image_path("LDRS", "comp", "A")

        command = [self.binary, "-cs", imIn, imOut, "6x6", "-exhaustive"]
        self.exec(command)
        self.assertTrue(filecmp.cmp(imRef, imOut, False))

    def test_hdr_compress1(self):
        """
        Test basic HDR + LDR alpha compression.
        """
        imIn = self.get_ref_image_path("HDR", "input", "A")
        imOut = self.get_tmp_image_path("HDR", "comp")
        imRef = self.get_ref_image_path("HDR", "comp", "A")

        command = [self.binary, "-ch", imIn, imOut, "6x6", "-exhaustive"]
        self.exec(command)
        self.assertTrue(filecmp.cmp(imRef, imOut, False))

    def test_hdr_compress2(self):
        """
        Test basic HDR + HDR alpha compression.
        """
        imIn = self.get_ref_image_path("HDR", "input", "A")
        imOut = self.get_tmp_image_path("HDR", "comp")
        imRef = self.get_ref_image_path("HDR", "comp", "A")

        command = [self.binary, "-cH", imIn, imOut, "6x6", "-exhaustive"]
        self.exec(command)
        self.assertTrue(filecmp.cmp(imRef, imOut, False))

    def test_ldr_decompress(self):
        """
        Test basic LDR decompression.
        """
        imIn = self.get_ref_image_path("LDR", "comp", "A")
        imOut = self.get_tmp_image_path("LDR", "decomp")
        imRef = self.get_ref_image_path("LDR", "input", "A")

        command = [self.binary, "-dl", imIn, imOut]
        self.exec(command)
        self.assertTrue(self.compare(imRef, imOut))

    def test_srgb_decompress(self):
        """
        Test basic LDR sRGB decompression.
        """
        imIn = self.get_ref_image_path("LDRS", "comp", "A")
        imOut = self.get_tmp_image_path("LDRS", "decomp")
        imRef = self.get_ref_image_path("LDRS", "input", "A")

        command = [self.binary, "-ds", imIn, imOut]
        self.exec(command)
        self.assertTrue(self.compare(imRef, imOut))

    def test_hdr_decompress1(self):
        """
        Test basic HDR + LDR alpha decompression.
        """
        imIn = self.get_ref_image_path("HDR", "comp", "A")
        imOut = self.get_tmp_image_path("HDR", "decomp")
        imRef = self.get_ref_image_path("HDR", "input", "A")

        command = [self.binary, "-dh", imIn, imOut]
        self.exec(command)

        colRef = tli.Image(imRef).get_colors((0, 0))
        colOut = tli.Image(imOut).get_colors((0, 0))
        self.assertColorSame(colRef, colOut)

    def test_hdr_decompress2(self):
        """
        Test basic HDR + HDR alpha decompression.
        """
        imIn = self.get_ref_image_path("HDR", "comp", "A")
        imOut = self.get_tmp_image_path("HDR", "decomp")
        imRef = self.get_ref_image_path("HDR", "input", "A")

        command = [self.binary, "-dH", imIn, imOut]
        self.exec(command)

        colRef = tli.Image(imRef).get_colors((0, 0))
        colOut = tli.Image(imOut).get_colors((0, 0))
        self.assertColorSame(colRef, colOut)

    def test_ldr_roundtrip(self):
        """
        Test basic LDR round-trip
        """
        imIn = self.get_ref_image_path("LDR", "input", "A")
        imOut = self.get_tmp_image_path("LDR", "decomp")

        command = [self.binary, "-tl", imIn, imOut, "6x6", "-exhaustive"]
        self.exec(command)
        self.assertTrue(self.compare(imIn, imOut))

    def test_srgb_roundtrip(self):
        """
        Test basic LDR sRGB round-trip
        """
        imIn = self.get_ref_image_path("LDRS", "input", "A")
        imOut = self.get_tmp_image_path("LDRS", "decomp")

        command = [self.binary, "-ts", imIn, imOut, "6x6", "-exhaustive"]
        self.exec(command)
        self.assertTrue(self.compare(imIn, imOut))

    def test_hdr_roundtrip1(self):
        """
        Test basic HDR + LDR alpha round-trip.
        """
        imIn = self.get_ref_image_path("HDR", "input", "A")
        imOut = self.get_tmp_image_path("HDR", "decomp")

        command = [self.binary, "-th", imIn, imOut, "6x6", "-exhaustive"]
        self.exec(command)
        colIn = tli.Image(imIn).get_colors((0, 0))
        colOut = tli.Image(imOut).get_colors((0, 0))
        self.assertColorSame(colIn, colOut)

    def test_hdr_roundtrip2(self):
        """
        Test basic HDR + HDR alpha round-trip.
        """
        imIn = self.get_ref_image_path("HDR", "input", "A")
        imOut = self.get_tmp_image_path("HDR", "decomp")

        command = [self.binary, "-tH", imIn, imOut, "6x6", "-exhaustive"]
        self.exec(command)
        colIn = tli.Image(imIn).get_colors((0, 0))
        colOut = tli.Image(imOut).get_colors((0, 0))
        self.assertColorSame(colIn, colOut)

    def test_valid_2d_block_sizes(self):
        """
        Test all valid block sizes are accepted (2D images).
        """
        blockSizes = [
            "4x4", "5x4", "5x5", "6x5", "6x6", "8x5", "8x6",
            "10x5", "10x6", "8x8", "10x8", "10x10", "12x10", "12x12"
        ]

        imIn = self.get_ref_image_path("LDR", "input", "A")
        imOut = self.get_tmp_image_path("LDR", "decomp")

        for blk in blockSizes:
            with self.subTest(blockSize=blk):
                command = [self.binary, "-tl", imIn, imOut, blk, "-exhaustive"]
                self.exec(command)
                colIn = tli.Image(imIn).get_colors((0, 0))
                colOut = tli.Image(imOut).get_colors((0, 0))
                self.assertColorSame(colIn, colOut)

    def test_valid_3d_block_sizes(self):
        """
        Test all valid block sizes are accepted (3D images).
        """
        blockSizes = [
            "3x3x3",
            "4x3x3", "4x4x3", "4x4x4",
            "5x4x4", "5x5x4", "5x5x5",
            "6x5x5", "6x6x5", "6x6x6"
        ]

        imIn = self.get_ref_image_path("LDR", "input", "A")
        imOut = self.get_tmp_image_path("LDR", "decomp")

        for blk in blockSizes:
            with self.subTest(blockSize=blk):
                command = [self.binary, "-tl", imIn, imOut, blk, "-exhaustive"]
                self.exec(command)
                colIn = tli.Image(imIn).get_colors((0, 0))
                colOut = tli.Image(imOut).get_colors((0, 0))
                self.assertColorSame(colIn, colOut)

    def test_valid_presets(self):
        """
        Test all valid presets are accepted
        """
        presets = ["-fastest", "-fast", "-medium",
                   "-thorough", "-verythorough", "-exhaustive"]

        imIn = self.get_ref_image_path("LDR", "input", "A")
        imOut = self.get_tmp_image_path("LDR", "decomp")

        for preset in presets:
            with self.subTest(preset=preset):
                command = [self.binary, "-tl", imIn, imOut, "4x4", preset]
                self.exec(command)
                colIn = tli.Image(imIn).get_colors((0, 0))
                colOut = tli.Image(imOut).get_colors((0, 0))
                self.assertColorSame(colIn, colOut)

    def test_valid_ldr_input_formats(self):
        """
        Test valid LDR input file formats.
        """
        imgFormats = ["bmp", "dds", "jpg", "ktx", "png", "tga"]

        for imgFormat in imgFormats:
            with self.subTest(imgFormat=imgFormat):
                imIn = "./Test/Data/Tiles/ldr.%s" % imgFormat
                imOut = self.get_tmp_image_path("LDR", "decomp")

                command = [self.binary, "-tl", imIn, imOut, "4x4", "-fast"]
                self.exec(command)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(imgFormat):
                    colIn = tli.Image(imIn).get_colors((7, 7))
                    colOut = tli.Image(imOut).get_colors((7, 7))

                    # Catch exception and add fallback for tga handling
                    # having unstable origin in ImageMagick
                    try:
                        self.assertColorSame(colIn, colOut)
                        continue
                    except AssertionError as ex:
                        if imgFormat != "tga":
                            raise ex

                    # Try yflipped TGA image
                    colIn = tli.Image(imIn).get_colors((7, 7))
                    colOut = tli.Image(imOut).get_colors((7, 1))
                    self.assertColorSame(colIn, colOut)

    def test_valid_uncomp_ldr_output_formats(self):
        """
        Test valid uncompressed LDR output file formats.
        """
        imgFormats = ["bmp", "dds", "ktx", "png", "tga"]

        for imgFormat in imgFormats:
            with self.subTest(imgFormat=imgFormat):
                imIn = self.get_ref_image_path("LDR", "input", "A")
                imOut = self.get_tmp_image_path("EXP", ".%s" % imgFormat)

                command = [self.binary, "-tl", imIn, imOut, "4x4", "-fast"]
                self.exec(command)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(imgFormat):
                    colIn = tli.Image(imIn).get_colors((7, 7))
                    colOut = tli.Image(imOut).get_colors((7, 7))
                    self.assertColorSame(colIn, colOut)

    def test_valid_comp_ldr_output_formats(self):
        """
        Test valid compressed LDR output file formats.
        """
        imgFormats = ["astc", "ktx"]

        for imgFormat in imgFormats:
            with self.subTest(imgFormat=imgFormat):
                imIn = self.get_ref_image_path("LDR", "input", "A")
                imOut = self.get_tmp_image_path("EXP", ".%s" % imgFormat)
                imOut2 = self.get_tmp_image_path("LDR", "decomp")

                command = [self.binary, "-cl", imIn, imOut, "4x4", "-fast"]
                self.exec(command)

                command = [self.binary, "-dl", imOut, imOut2]
                self.exec(command)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(imgFormat):
                    colIn = tli.Image(imIn).get_colors((7, 7))
                    colOut = tli.Image(imOut2).get_colors((7, 7))
                    self.assertColorSame(colIn, colOut2)

    def test_valid_hdr_input_formats(self):
        """
        Test valid HDR input file formats.
        """
        imgFormats = ["exr", "hdr"]

        for imgFormat in imgFormats:
            with self.subTest(imgFormat=imgFormat):
                imIn = "./Test/Data/Tiles/hdr.%s" % imgFormat
                imOut = self.get_tmp_image_path("HDR", "decomp")

                command = [self.binary, "-th", imIn, imOut, "4x4", "-fast"]
                self.exec(command)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(imgFormat, profile="hdr"):
                    colIn = tli.Image(imIn).get_colors((7, 7))
                    colOut = tli.Image(imOut).get_colors((7, 7))
                    self.assertColorSame(colIn, colOut)

    def test_valid_uncomp_hdr_output_formats(self):
        """
        Test valid uncompressed HDR output file formats.
        """
        imgFormats = ["dds", "exr", "hdr", "ktx"]

        for imgFormat in imgFormats:
            with self.subTest(imgFormat=imgFormat):
                imIn = self.get_ref_image_path("HDR", "input", "A")
                imOut = self.get_tmp_image_path("EXP", ".%s" % imgFormat)

                command = [self.binary, "-th", imIn, imOut, "4x4", "-fast"]
                self.exec(command)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(imgFormat, profile="hdr"):
                    colIn = tli.Image(imIn).get_colors((7, 7))
                    colOut = tli.Image(imOut).get_colors((7, 7))
                    self.assertColorSame(colIn, colOut)

    def test_valid_comp_hdr_output_formats(self):
        """
        Test valid compressed HDR output file formats.
        """
        imgFormats = ["astc", "ktx"]

        for imgFormat in imgFormats:
            with self.subTest(imgFormat=imgFormat):
                imIn = self.get_ref_image_path("HDR", "input", "A")
                imOut = self.get_tmp_image_path("EXP", ".%s" % imgFormat)
                imOut2 = self.get_tmp_image_path("HDR", "decomp")

                command = [self.binary, "-ch", imIn, imOut, "4x4", "-fast"]
                self.exec(command)

                command = [self.binary, "-dh", imOut, imOut2]
                self.exec(command)

                # Check colors if image wrapper supports it
                if tli.Image.is_format_supported(imgFormat):
                    colIn = tli.Image(imIn).get_colors((7, 7))
                    colOut = tli.Image(imOut2).get_colors((7, 7))
                    self.assertColorSame(colIn, colOut2)

    def test_compress_normal_psnr(self):
        """
        Test compression of normal textures using PSNR error metrics.
        """
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        command = [
            self.binary, "-tl",
            "./Test/Images/Small/LDR-XY/ldr-xy-00.png",
            decompFile, "5x5", "-exhaustive"]

        refdB = float(self.exec(command, LDR_RGB_PSNR_PATTERN))

        command.append("-normal")
        testdB = float(self.exec(command, LDR_RGB_PSNR_PATTERN))

        # Note that this test simply asserts that the "-normal_psnr" is
        # connected and affects the output. We don't test it does something
        # useful; that it outside the scope of this test case.
        self.assertNotEqual(refdB, testdB)

    def test_compress_normal_percep(self):
        """
        Test compression of normal textures using perceptual error metrics.
        """
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        command = [
            self.binary, "-tl",
            "./Test/Images/Small/LDR-XY/ldr-xy-00.png",
            decompFile, "4x4", "-exhaustive"]

        refdB = float(self.exec(command, LDR_RGB_PSNR_PATTERN))

        command.append("-normal")
        command.append("-perceptual")
        testdB = float(self.exec(command, LDR_RGB_PSNR_PATTERN))

        # Note that this test simply asserts that the "-normal -percep" is
        # connected and affects the output. We don't test it does something
        # useful; that it outside the scope of this test case.
        self.assertNotEqual(refdB, testdB)

    def test_compress_esw(self):
        """
        Test compression swizzles.
        """
        # The swizzles to test
        swizzles = ["rgba", "g0r1", "rrrg"]

        # Compress a swizzled image
        for swizzle in swizzles:
            with self.subTest(swizzle=swizzle):
                decompFile = self.get_tmp_image_path("LDR", "decomp")

                command = [
                    self.binary, "-tl",
                    "./Test/Data/Tiles/ldr.png",
                    decompFile, "4x4", "-exhaustive",
                    "-esw", swizzle]

                self.exec(command)

                # Fetch the three color
                img = tli.Image(decompFile)
                colorVal = img.get_colors([(7, 7)])
                colorRef = self.get_color_refs("LDR", "BR")
                self.assertColorSame(colorRef[0], colorVal[0], swiz=swizzle)

    def test_compress_dsw(self):
        """
        Test decompression swizzles.
        """
        # The swizzles to test
        swizzles = ["rgba", "g0r1", "rrrg"]

        # Decompress a swizzled image
        for swizzle in swizzles:
            with self.subTest(swizzle=swizzle):
                decompFile = self.get_tmp_image_path("LDR", "decomp")

                command = [
                    self.binary, "-tl",
                    "./Test/Data/Tiles/ldr.png",
                    decompFile, "4x4", "-exhaustive",
                    "-dsw", swizzle]

                self.exec(command)

                # Fetch the three color
                img = tli.Image(decompFile)
                colorVal = img.get_colors([(7, 7)])
                colorRef = self.get_color_refs("LDR", "BR")
                self.assertColorSame(colorRef[0], colorVal[0], swiz=swizzle)

    def test_compress_esw_dsw(self):
        """
        Test compression and decompression swizzles
        """
        # Compress a swizzled image, and swizzle back in decompression
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        command = [
            self.binary, "-tl",
            "./Test/Data/Tiles/ldr.png",
            decompFile, "4x4", "-exhaustive",
            "-esw", "gbar", "-dsw", "argb"]

        self.exec(command)

        # Fetch the three color
        img = tli.Image(decompFile)
        colorVal = img.get_colors([(7, 7)])
        colorRef = self.get_color_refs("LDR", "BR")
        self.assertColorSame(colorRef[0], colorVal[0])

    def test_compress_flip(self):
        """
        Test LDR image flip on compression.
        """
        # Compress a flipped image
        compFile = self.get_tmp_image_path("LDR", "comp")

        command = [
            self.binary, "-cl",
            "./Test/Data/Tiles/ldr.png",
            compFile, "4x4", "-fast", "-yflip"]

        self.exec(command)

        # Decompress a non-flipped image
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        command = [
            self.binary, "-dl",
            compFile,
            decompFile]

        self.exec(command)

        # Compare TL (0, 0) with BL - should match
        colorRef = self.get_color_refs("LDR", "BL")

        img = tli.Image(decompFile)
        colorVal = img.get_colors([(0, 0)])
        self.assertColorSame(colorRef[0], colorVal[0])

    def test_decompress_flip(self):
        """
        Test LDR image flip on decompression.
        """
        # Compress a non-flipped image
        compFile = self.get_tmp_image_path("LDR", "comp")

        command = [
            self.binary, "-cl",
            "./Test/Data/Tiles/ldr.png",
            compFile, "4x4", "-fast"]

        self.exec(command)

        # Decompress a flipped image
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        command = [
            self.binary, "-dl",
            compFile,
            decompFile, "-yflip"]

        self.exec(command)

        # Compare TL (0, 0) with BL - should match
        colorRef = self.get_color_refs("LDR", "BL")

        img = tli.Image(decompFile)
        colorVal = img.get_colors([(0, 0)])
        self.assertColorSame(colorRef[0], colorVal[0])

    def test_roundtrip_flip(self):
        """
        Test LDR image flip on roundtrip (no flip should occur).
        """
        # Compress and decompressed a flipped LDR image
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        command = [
            self.binary, "-tl",
            "./Test/Data/Tiles/ldr.png",
            decompFile, "4x4", "-fast", "-yflip"]

        self.exec(command)

        # Compare TL (0, 0) with TL - should match - i.e. no flip
        colorRef = self.get_color_refs("LDR", "TL")

        img = tli.Image(decompFile)
        colorVal = img.get_colors([(0, 0)])

        self.assertColorSame(colorRef[0], colorVal[0])

    def test_channel_weighting(self):
        """
        Test channel weighting.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        baseRMSE = self.get_channel_rmse(inputFile, decompFile)

        # Note: Using -cw can result in a worse result than not using -cw,
        # with regressions in RMSE for the high-weighted channel. This is
        # particularly an issue in synthetic images, as they are more likely to
        # hit corner cases in the heuristics. It happens to "work" for the
        # selected test image and these settings, but might start to fail in
        # future due to compressor changes.

        # Test each channel with a high weight
        for chIdx, chName in ((0, "R"), (1, "G"), (2, "B"), (3, "A")):
            with self.subTest(channel=chName):
                cwArg = ["%s" % (10 if x == chIdx else 1) for x in range(0, 4)]
                command2 = command + ["-cw"] + cwArg
                self.exec(command2)
                chRMSE = self.get_channel_rmse(inputFile, decompFile)
                self.assertLess(chRMSE[chIdx], baseRMSE[chIdx])

    def test_partition_count_limit(self):
        """
        Test partition count limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-partitioncountlimit", "1"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_2partition_index_limit(self):
        """
        Test partition index limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-2partitionindexlimit", "1"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_3partition_index_limit(self):
        """
        Test partition index limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-3partitionindexlimit", "1"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_4partition_index_limit(self):
        """
        Test partition index limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-4partitionindexlimit", "1"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_blockmode_limit(self):
        """
        Test block mode limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-blockmodelimit", "25"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_refinement_limit(self):
        """
        Test refinement limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-refinementlimit", "1"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_candidate_limit(self):
        """
        Test candidate limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-candidatelimit", "1"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_db_cutoff_limit(self):
        """
        Test db cutoff limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-dblimit", "10"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce cutoff quality
        self.assertGreater(testRMSE, refRMSE)

    def test_2partition_early_limit(self):
        """
        Test 2 partition early limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-2partitionlimitfactor", "1.0"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_3partition_early_limit(self):
        """
        Test 3 partition early limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-3partitionlimitfactor", "1.0"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertNotEqual(testRMSE, refRMSE)

    def test_2plane_correlation_limit(self):
        """
        Test 2 plane correlation limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-2planelimitcorrelation", "0.1"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_2partition_candidate_limit(self):
        """
        Test 2 partition partitioning candidate limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-2partitioncandidatelimit", "1"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_3partition_candidate_limit(self):
        """
        Test 3 partition partitioning candidate limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-3partitioncandidatelimit", "1"]
        self.exec(command)
        testRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        # RMSE should get worse (higher) if we reduce search space
        self.assertGreater(testRMSE, refRMSE)

    def test_4partition_candidate_limit(self):
        """
        Test 4 partition partitioning candidate limit.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        self.exec(command)
        refRMSE = sum(self.get_channel_rmse(inputFile, decompFile))

        command += ["-4partitioncandidatelimit", "1"]
        self.exec(command)

        # RMSE should get worse (higher) if we reduce search space
        # Don't check this here, as 4 partitions not used in any Small image
        # even for -exhaustive, BUT command line option must be accepted and
        # not error ...
        # self.assertGreater(testRMSE, refRMSE)

    @unittest.skipIf(os.cpu_count() == 1, "Cannot test on single core host")
    def test_thread_count(self):
        """
        Test codec thread count.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]

        start = time.time()
        self.exec(command)
        refTime = time.time() - start

        command += ["-j", "1"]
        start = time.time()
        self.exec(command)
        testTime = time.time() - start

        # Test time should get slower with fewer threads
        self.assertGreater(testTime, refTime)

    def test_silent(self):
        """
        Test silent
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        decompFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the basic image without any channel weights
        command = [
            self.binary, "-tl",
            inputFile, decompFile, "4x4", "-medium"]
        stdout = self.exec(command)

        command += ["-silent"]
        stdoutSilent = self.exec(command)

        # Check that stdout is shorter in silent mode. Note that this doesn't
        # check that it is as silent as it should be, just that silent is wired
        # somewhere ...
        self.assertLess(len(stdoutSilent), len(stdout))

    def test_image_quality_stability(self):
        """
        Test that a round-trip and a file-based round-trip give same result.
        """
        inputFile = "./Test/Images/Small/LDR-RGBA/ldr-rgba-00.png"
        p1DecFile = self.get_tmp_image_path("LDR", "decomp")
        p2CompFile = self.get_tmp_image_path("LDR", "comp")
        p2DecFile = self.get_tmp_image_path("LDR", "decomp")

        # Compute the first image using a direct round-trip
        command = [self.binary, "-tl", inputFile, p1DecFile, "4x4", "-medium"]
        self.exec(command)

        # Compute the first image using a file-based round-trip
        command = [self.binary, "-cl", inputFile, p2CompFile, "4x4", "-medium"]
        self.exec(command)
        command = [self.binary, "-dl", p2CompFile, p2DecFile]
        self.exec(command)

        # RMSE should be the same
        p1RMSE = sum(self.get_channel_rmse(inputFile, p1DecFile))
        p2RMSE = sum(self.get_channel_rmse(inputFile, p2DecFile))
        self.assertEqual(p1RMSE, p2RMSE)


class CLINTest(CLITestBase):
    """
    Command line interface negative tests.

    These tests are designed to test that bad inputs to the command line are
    handled cleanly and that errors are correctly thrown.

    Note that many tests are mutations of a valid positive test command line,
    to ensure that the base command line is valid before it is mutated many
    of these tests include a *positive test* to ensure that the starting point
    is actually a valid command line (otherwise we could be throwing an
    arbitrary error).
    """

    def exec(self, command, expectPass=False):
        """
        Execute a negative test.

        Test will automatically fail if:

        * The subprocess return code is zero, unless ``expectPass==True``.
        * The subprocess correctly returned non-zero, but without any error
          message.
        * The subprocess dies with any kind of signal.

        Args:
            command (list(str)): The command to execute.
            expectPass (bool): ``True`` if this command is actually expected to
                pass, which is used to validate commands before mutating them.
        """
        try:
            result = sp.run(command, stdout=sp.PIPE, stderr=sp.PIPE,
                            universal_newlines=True, check=True)
            error = False
        except sp.CalledProcessError as ex:
            # Pop out of the CPE scope to handle the error, as this reduces
            # test log verbosity on failure by avoiding nested exceptions
            result = ex
            error = True

        rcode = result.returncode

        # Emit debug logging if needed (negative rcode is a signal)
        badResult = (error == expectPass) or (rcode < 0)

        if ASTCENC_CLI_ALWAYS or (badResult and ASTCENC_CLI_ON_ERROR_NEG):
            # Format for shell replay
            print("\n" + " ".join(command))
            # Format for script command list replay
            print("\n" + ", ".join(("\"%s\"" % x for x in command)))

        if ASTCENC_LOG_ALWAYS or (badResult and ASTCENC_LOG_ON_ERROR_NEG):
            print(result.stdout)

        # If we expected a pass, then rcode == 0
        if expectPass:
            self.assertEqual(rcode, 0, "Exec did not pass as expected")
            self.assertNotIn("ERROR", result.stderr)
            return

        # If we got a negative that's always bad (signal of some kind)
        if rcode < 0:
            msg = "Exec died with signal %s" % signal.Signals(-rcode).name
            self.assertGreaterEqual(rcode, 0, msg)

        # Otherwise just assert that we got an error log, and some positive
        # return code value was returned
        self.assertIn("ERROR", result.stderr)
        self.assertGreater(rcode, 0, "Exec did not fail as expected")

    def exec_with_omit(self, command, startOmit):
        """
        Execute a negative test with command line argument omission.

        These tests aim to prove that the command fails if arguments are
        missing. However the passed command MUST be a valid command which
        passes if no argument are omitted (this is checked, to ensure that
        the test case is a valid test).

        Test will automatically fail if:

        * A partial command doesn't fail.
        * The full command doesn't pass.
        """
        # Run the command, incrementally omitting arguments
        commandLen = len(command)
        for subLen in range(startOmit, commandLen + 1):
            omit = len(command) - subLen
            with self.subTest(omit=omit):
                testCommand = command[:subLen]
                expectPass = omit == 0
                self.exec(testCommand, expectPass)

    def test_cl_missing_args(self):
        """
        Test -cl with missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast"]

        self.exec_with_omit(command, 2)

    def test_cl_missing_input(self):
        """
        Test -cl with a missing input file.
        """
        # Build a valid command with a missing input file
        command = [
            self.binary, "-cl",
            "./Test/Data/missing.png",
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast"]

        self.exec(command)

    def test_cl_missing_input_array_slice(self):
        """
        Test -cl with a missing input file in an array slice.
        """
        # Build a valid command with a missing input file
        command = [
            self.binary, "-cl",
            "./Test/Data/Tiles/ldr.png",
            self.get_tmp_image_path("LDR", "comp"),
            "3x3x3", "-fast", "-zdim", "3"]

        self.exec(command)

    def test_cl_unknown_input(self):
        """
        Test -cl with an unknown input file extension.
        """
        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-cl",
            "./Test/Data/empty.unk",
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast"]

        self.exec(command)

    def test_cl_missing_output(self):
        """
        Test -cl with a missing output directory.
        """
        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            "./DoesNotExist/test.astc",
            "4x4", "-fast"]

        self.exec(command)

    def test_cl_unknown_output(self):
        """
        Test -cl with an unknown output file extension.
        """
        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            "./test.aastc",
            "4x4", "-fast"]

        self.exec(command)

    def test_cl_bad_block_size(self):
        """
        Test -cl with an invalid block size.
        """
        badSizes = [
            "4x5",      # Illegal 2D block size
            "3x3x4",    # Illegal 3D block size
            "4x4x4x4",  # Too many dimensions
            "4x",       # Incomplete 2D block size
            "4x4x",     # Incomplete 3D block size
            "4x4x4x",   # Over-long 3D block size
            "4xe",      # Illegal non-numeric character
            "4x4e"      # Additional non-numeric character
        ]

        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast"]

        # Test that the underlying command is valid
        self.exec(command, True)

        blockIndex = command.index("4x4")
        for badSize in badSizes:
            with self.subTest(blockSize=badSize):
                command[blockIndex] = badSize
                self.exec(command)

    def test_cl_bad_preset(self):
        """
        Test -cl with an invalid encoding preset.
        """
        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fastt"]

        self.exec(command)

    def test_cl_bad_argument(self):
        """
        Test -cl with an unknown additional argument.
        """
        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast", "-unknown"]

        self.exec(command)

    def test_cl_2d_block_with_array(self):
        """
        Test -cl with a 2D block size and 3D input data.
        """
        # Build an otherwise valid command with the test flaw

        # TODO: This fails late (i.e. the data is still loaded, and we fail
        # at processing time when we see a 3D array). We could fail earlier at
        # parse time, which might consolidate the error handling code.
        command = [
            self.binary, "-cl",
            "./Test/Data/Tiles/ldr.png",
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast", "-zdim", "2"]

        self.exec(command)

    def test_cl_array_missing_args(self):
        """
        Test -cl with a 2D block size and 3D input data.
        """
        # Build an otherwise valid command
        command = [
            self.binary, "-cl",
            "./Test/Data/Tiles/ldr.png",
            self.get_tmp_image_path("LDR", "comp"),
            "4x4x4", "-fast", "-zdim", "2"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_tl_missing_args(self):
        """
        Test -tl with missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-tl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "decomp"),
            "4x4", "-fast"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 2)

    def test_tl_missing_input(self):
        """
        Test -tl with a missing input file.
        """
        # Build a valid command with a missing input file
        command = [
            self.binary, "-tl",
            "./Test/Data/missing.png",
            self.get_tmp_image_path("LDR", "decomp"),
            "4x4", "-fast"]

        self.exec(command)

    def test_tl_unknown_input(self):
        """
        Test -tl with an unknown input file extension.
        """
        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-tl",
            "./Test/Data/empty.unk",
            self.get_tmp_image_path("LDR", "decomp"),
            "4x4", "-fast"]

        self.exec(command)

    def test_tl_missing_output(self):
        """
        Test -tl with a missing output directory.
        """
        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-tl",
            self.get_ref_image_path("LDR", "input", "A"),
            "./DoesNotExist/test.png",
            "4x4", "-fast"]

        self.exec(command)

    def test_tl_bad_block_size(self):
        """
        Test -tl with an invalid block size.
        """
        badSizes = [
            "4x5",      # Illegal 2D block size
            "3x3x4",    # Illegal 3D block size
            "4x4x4x4",  # Too many dimensions
            "4x",       # Incomplete 2D block size
            "4x4x",     # Incomplete 3D block size
            "4x4x4x",   # Over-long 3D block size
            "4xe",      # Illegal non-numeric character
            "4x4e"      # Additional non-numeric character
        ]

        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-tl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "decomp"),
            "4x4", "-fast"]

        # Test that the underlying command is valid
        self.exec(command, True)

        blockIndex = command.index("4x4")
        for badSize in badSizes:
            with self.subTest(blockSize=badSize):
                command[blockIndex] = badSize
                self.exec(command)

    def test_tl_bad_preset(self):
        """
        Test -tl with an invalid encoding preset.
        """
        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-tl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "decomp"),
            "4x4", "-fastt"]

        self.exec(command)

    def test_tl_bad_argument(self):
        """
        Test -tl with an unknown additional argument.
        """
        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-tl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "decomp"),
            "4x4", "-fast", "-unknown"]

        self.exec(command)

    def test_dl_missing_args(self):
        """
        Test -dl with missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-dl",
            self.get_ref_image_path("LDR", "comp", "A"),
            self.get_tmp_image_path("LDR", "decomp")]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 2)

    def test_dl_missing_output(self):
        """
        Test -dl with a missing output directory.
        """
        # Build an otherwise valid command with the test flaw
        command = [
            self.binary, "-dl",
            self.get_ref_image_path("LDR", "comp", "A"),
            "./DoesNotExist/test.png"]

        self.exec(command)

    def test_cl_a_missing_args(self):
        """
        Test -cl with -a and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-a", "2"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_cw_missing_args(self):
        """
        Test -cl with -cw and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-cw", "0", "1", "2", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_2partitionindexlimit_missing_args(self):
        """
        Test -cl with -2partitionindexlimit and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-2partitionindexlimit", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_3partitionindexlimit_missing_args(self):
        """
        Test -cl with -3partitionindexlimit and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-3partitionindexlimit", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_4partitionindexlimit_missing_args(self):
        """
        Test -cl with -4partitionindexlimit and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-4partitionindexlimit", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_2partitioncandidatelimit_missing_args(self):
        """
        Test -cl with -2partitioncandidatelimit and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-2partitioncandidatelimit", "1"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_3partitioncandidatelimit_missing_args(self):
        """
        Test -cl with -3partitioncandidatelimit and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-3partitioncandidatelimit", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)


    def test_cl_4partitioncandidatelimit_missing_args(self):
        """
        Test -cl with -4partitioncandidatelimit and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-4partitioncandidatelimit", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_blockmodelimit_missing_args(self):
        """
        Test -cl with -blockmodelimit and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-blockmodelimit", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_refinementlimit_missing_args(self):
        """
        Test -cl with -refinementlimit and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-refinementlimit", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_dblimit_missing_args(self):
        """
        Test -cl with -dblimit and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-dblimit", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_2partitionearlylimit_missing_args(self):
        """
        Test -cl with -2partitionlimitfactor and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-2partitionlimitfactor", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_3partitionearlylimit_missing_args(self):
        """
        Test -cl with -3partitionlimitfactor and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-3partitionlimitfactor", "3"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_2planeearlylimit_missing_args(self):
        """
        Test -cl with -2planelimitcorrelation and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-2planelimitcorrelation", "0.66"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_esw_missing_args(self):
        """
        Test -cl with -esw and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-esw", "rgb1"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_esw_invalid_swizzle(self):
        """
        Test -cl with -esw and invalid swizzles.
        """
        badSwizzles = [
            "",  # Short swizzles
            "r",
            "rr",
            "rrr",
            "rrrrr",  # Long swizzles
        ]

        # Create swizzles with all invalid printable ascii codes
        good = ["r", "g", "b", "a", "0", "1"]
        for channel in string.printable:
            if channel not in good:
                badSwizzles.append(channel * 4)

        # Build a valid base command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-esw", "rgba"]

        blockIndex = command.index("rgba")
        for badSwizzle in badSwizzles:
            with self.subTest(swizzle=badSwizzle):
                command[blockIndex] = badSwizzle
                self.exec(command)

    def test_cl_ssw_missing_args(self):
        """
        Test -cl with -ssw and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-ssw", "rgba"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)

    def test_cl_ssw_invalid_swizzle(self):
        """
        Test -cl with -ssw and invalid swizzles.
        """
        badSwizzles = [
            "",  # Short swizzles
            "rrrrr",  # Long swizzles
        ]

        # Create swizzles with all invalid printable ascii codes
        good = ["r", "g", "b", "a"]
        for channel in string.printable:
            if channel not in good:
                badSwizzles.append(channel * 4)

        # Build a valid base command
        command = [
            self.binary, "-cl",
            self.get_ref_image_path("LDR", "input", "A"),
            self.get_tmp_image_path("LDR", "comp"),
            "4x4", "-fast",
            "-ssw", "rgba"]

        blockIndex = command.index("rgba")
        for badSwizzle in badSwizzles:
            with self.subTest(swizzle=badSwizzle):
                command[blockIndex] = badSwizzle
                self.exec(command)

    def test_dl_dsw_missing_args(self):
        """
        Test -dl with -dsw and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-dl",
            self.get_ref_image_path("LDR", "comp", "A"),
            self.get_tmp_image_path("LDR", "decomp"),
            "-dsw", "rgb1"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 5)

    def test_dl_dsw_invalid_swizzle(self):
        """
        Test -dl with -dsw and invalid swizzles.
        """
        badSwizzles = [
            "",  # Short swizzles
            "r",
            "rr",
            "rrr",
            "rrrrr",  # Long swizzles
        ]

        # Create swizzles with all invalid printable ascii codes
        good = ["r", "g", "b", "a", "z", "0", "1"]
        for channel in string.printable:
            if channel not in good:
                badSwizzles.append(channel * 4)

        # Build a valid base command
        command = [
            self.binary, "-dl",
            self.get_ref_image_path("LDR", "comp", "A"),
            self.get_tmp_image_path("LDR", "decomp"),
            "-dsw", "rgba"]

        blockIndex = command.index("rgba")
        for badSwizzle in badSwizzles:
            with self.subTest(swizzle=badSwizzle):
                command[blockIndex] = badSwizzle
                self.exec(command)

    def test_ch_mpsnr_missing_args(self):
        """
        Test -ch with -mpsnr and missing arguments.
        """
        # Build a valid command
        command = [
            self.binary, "-ch",
            self.get_ref_image_path("HDR", "input", "A"),
            self.get_tmp_image_path("HDR", "comp"),
            "4x4", "-fast",
            "-mpsnr", "-5", "5"]

        # Run the command, incrementally omitting arguments
        self.exec_with_omit(command, 7)


def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """
    global g_TestEncoder

    parser = argparse.ArgumentParser()

    coders = ["none", "neon", "sse2", "sse4.1", "avx2"]
    parser.add_argument("--encoder", dest="encoder", default="avx2",
                        choices=coders, help="test encoder variant")
    args = parser.parse_known_args()

    # Set the encoder for this test run
    g_TestEncoder = args[0].encoder

    # Set the sys.argv to remaining args (leave sys.argv[0] alone)
    sys.argv[1:] = args[1]

    results = unittest.main(exit=False)
    return 0 if results.result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(main())
