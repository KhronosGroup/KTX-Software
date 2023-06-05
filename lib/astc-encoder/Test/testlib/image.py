# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2019-2022 Arm Limited
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
This module contains code for loading image metadata from a file path on disk.

The directory path is structured:

    TestSetName/TestFormat/FileName

... and the file name is structured:

    colorProfile-colorFormat-name[-flags].extension
"""

from collections.abc import Iterable
import os
import re
import subprocess as sp

from PIL import Image as PILImage

import testlib.misc as misc


CONVERT_BINARY = ["convert"]


class ImageException(Exception):
    """
    Exception thrown for bad image specification.
    """


class TestImage():
    """
    Objects of this type contain metadata for a single test image on disk.

    Attributes:
        filePath: The path of the file on disk.
        outFilePath: The path of the output file on disk.
        testSet: The name of the test set.
        testFormat: The test format group.
        testFile: The test file name.
        colorProfile: The image compression color profile.
        colorFormat: The image color format.
        name: The image human name.
        is3D: True if the image is 3D, else False.
        isAlphaScaled: True if the image wants alpha scaling, else False.
        TEST_EXTS: Expected test image extensions.
        PROFILES: Tuple of valid color profile values.
        FORMATS: Tuple of valid color format values.
        FLAGS: Map of valid flags (key) and their meaning (value).
    """
    TEST_EXTS = (".jpg", ".png", ".tga", ".dds", ".hdr", ".ktx")

    PROFILES = ("ldr", "ldrs", "hdr")

    FORMATS = ("l", "la", "xy", "rgb", "rgba")

    FLAGS = {
        # Flags for image compression control
        "3": "3D image",
        "m": "Mask image",
        "a": "Alpha scaled image"
    }

    def __init__(self, filePath):
        """
        Create a new image definition, based on a structured file path.

        Args:
            filePath (str): The path of the image on disk.

        Raises:
            ImageException: The image couldn't be found or is unstructured.
        """
        self.filePath = os.path.abspath(filePath)
        if not os.path.exists(self.filePath):
            raise ImageException("Image doesn't exist (%s)" % filePath)

        # Decode the path
        scriptDir = os.path.dirname(__file__)
        rootInDir = os.path.join(scriptDir, "..", "Images")
        partialPath = os.path.relpath(self.filePath, rootInDir)
        parts = misc.path_splitall(partialPath)
        if len(parts) != 3:
            raise ImageException("Image path not path triplet (%s)" % parts)
        self.testSet = parts[0]
        self.testFormat = parts[1]
        self.testFile = parts[2]

        # Decode the file name
        self.decode_file_name(self.testFile)

        # Output file path (store base without extension)
        rootOutDir = os.path.join(scriptDir, "..", "..", "TestOutput")
        outFilePath = os.path.join(rootOutDir, partialPath)
        outFilePath = os.path.abspath(outFilePath)
        outFilePath = os.path.splitext(outFilePath)[0]
        self.outFilePath = outFilePath

    def decode_file_name(self, fileName):
        """
        Utility function to decode metadata from an encoded file name.

        Args:
            fileName (str): The file name to tokenize.

        Raises:
            ImageException: The image file path is badly structured.
        """
        # Strip off the extension
        rootName = os.path.splitext(fileName)[0]

        parts = rootName.split("-")

        # Decode the mandatory fields
        if len(parts) >= 3:
            self.colorProfile = parts[0]
            if self.colorProfile not in self.PROFILES:
                raise ImageException("Unknown color profile (%s)" % parts[0])

            self.colorFormat = parts[1]
            if self.colorFormat not in self.FORMATS:
                raise ImageException("Unknown color format (%s)" % parts[1])

            # Consistency check between directory and file names
            reencode = "%s-%s" % (self.colorProfile, self.colorFormat)
            compare = self.testFormat.lower()
            if reencode != compare:
                dat = (self.testFormat, reencode)
                raise ImageException("Mismatched test and image (%s:%s)" % dat)

            self.name = parts[2]

        # Set default values for the optional fields
        self.is3D = False
        self.isAlphaScaled = False

        # Decode the flags field if present
        if len(parts) >= 4:
            flags = parts[3]
            seenFlags = set()
            for flag in flags:
                if flag in seenFlags:
                    raise ImageException("Duplicate flag (%s)" % flag)
                if flag not in self.FLAGS:
                    raise ImageException("Unknown flag (%s)" % flag)
                seenFlags.add(flag)

            self.is3D = "3" in seenFlags
            self.isAlphaScaled = "a" in seenFlags

    def get_size(self):
        """
        Get the dimensions of this test image, if format is known.

        Known cases today where the format is not known:

        * 3D .dds files.
        * Any .ktx, .hdr, .exr, or .astc file.

        Returns:
            tuple(int, int): The dimensions of a 2D image, or ``None`` if PIL
            could not open the file.
        """
        try:
            img = PILImage.open(self.filePath)
        except IOError:
            # HDR files
            return None
        except NotImplementedError:
            # DDS files
            return None

        return (img.size[0], img.size[1])


class Image():
    """
    Wrapper around an image on the file system.
    """

    # TODO: We don't support KTX yet, as ImageMagick doesn't.
    SUPPORTED_LDR = ["bmp", "jpg", "png", "tga"]
    SUPPORTED_HDR = ["exr", "hdr"]

    @classmethod
    def is_format_supported(cls, fileFormat, profile=None):
        """
        Test if a given file format is supported by the library.

        Args:
            fileFormat (str): The file extension (excluding the ".").
            profile (str or None): The profile (ldr or hdr) of the image.

        Returns:
            bool: `True` if the image is supported, `False` otherwise.
        """
        assert profile in [None, "ldr", "hdr"]

        if profile == "ldr":
            return fileFormat in cls.SUPPORTED_LDR

        if profile == "hdr":
            return fileFormat in cls.SUPPORTED_HDR

        return fileFormat in cls.SUPPORTED_LDR or \
            fileFormat in cls.SUPPORTED_HDR

    def __init__(self, filePath):
        """
        Construct a new Image.

        Args:
            filePath (str): The path to the image on disk.
        """
        self.filePath = filePath
        self.proxyPath = None

    def get_colors(self, coords):
        """
        Get the image colors at the given coordinate.

        Args:
            coords (tuple or list): A single coordinate, or a list of
                coordinates to sample.

        Returns:
            tuple: A single sample color (if `coords` was a coordinate).
            list: A list of sample colors (if `coords` was a list).

            Colors are returned as float values between 0.0 and 1.0 for LDR,
            and float values which may exceed 1.0 for HDR.
        """
        colors = []

        # We accept both a list of positions and a single position;
        # canonicalize here so the main processing only handles lists
        isList = len(coords) != 0 and isinstance(coords[0], Iterable)

        if not isList:
            coords = [coords]

        for (x, y) in coords:
            command = list(CONVERT_BINARY)
            command += [self.filePath]

            # Ensure convert factors in format origin if needed
            command += ["-auto-orient"]

            command += [
                "-format", "%%[pixel:p{%u,%u}]" % (x, y),
                "info:"
            ]

            if os.name == 'nt':
                command.insert(0, "magick")

            result = sp.run(command, stdout=sp.PIPE, stderr=sp.PIPE,
                            check=True, universal_newlines=True)

            rawcolor = result.stdout.strip()

            # Decode ImageMagick's annoying named color outputs. Note that this
            # only handles "known" cases triggered by our test images, we don't
            # support the entire ImageMagick named color table.
            if rawcolor == "black":
                colors.append([0.0, 0.0, 0.0, 1.0])
            elif rawcolor == "white":
                colors.append([1.0, 1.0, 1.0, 1.0])
            elif rawcolor == "red":
                colors.append([1.0, 0.0, 0.0, 1.0])
            elif rawcolor == "blue":
                colors.append([0.0, 0.0, 1.0, 1.0])

            # Decode ImageMagick's format tuples
            elif rawcolor.startswith("srgba"):
                rawcolor = rawcolor[6:]
                rawcolor = rawcolor[:-1]
                channels = rawcolor.split(",")
                for i, channel in enumerate(channels):
                    if (i < 3) and channel.endswith("%"):
                        channels[i] = float(channel[:-1]) / 100.0
                    elif (i < 3) and not channel.endswith("%"):
                        channels[i] = float(channel) / 255.0
                    else:
                        channels[i] = float(channel)
                colors.append(channels)
            elif rawcolor.startswith("srgb"):
                rawcolor = rawcolor[5:]
                rawcolor = rawcolor[:-1]
                channels = rawcolor.split(",")
                for i, channel in enumerate(channels):
                    if (i < 3) and channel.endswith("%"):
                        channels[i] = float(channel[:-1]) / 100.0
                    if (i < 3) and not channel.endswith("%"):
                        channels[i] = float(channel) / 255.0
                channels.append(1.0)
                colors.append(channels)
            elif rawcolor.startswith("rgba"):
                rawcolor = rawcolor[5:]
                rawcolor = rawcolor[:-1]
                channels = rawcolor.split(",")
                for i, channel in enumerate(channels):
                    if (i < 3) and channel.endswith("%"):
                        channels[i] = float(channel[:-1]) / 100.0
                    elif (i < 3) and not channel.endswith("%"):
                        channels[i] = float(channel) / 255.0
                    else:
                        channels[i] = float(channel)
                colors.append(channels)
            elif rawcolor.startswith("rgb"):
                rawcolor = rawcolor[4:]
                rawcolor = rawcolor[:-1]
                channels = rawcolor.split(",")
                for i, channel in enumerate(channels):
                    if (i < 3) and channel.endswith("%"):
                        channels[i] = float(channel[:-1]) / 100.0
                    if (i < 3) and not channel.endswith("%"):
                        channels[i] = float(channel) / 255.0
                channels.append(1.0)
                colors.append(channels)
            else:
                print(x, y)
                print(rawcolor)
                assert False

        # ImageMagick decodes DDS files as BGRA not RGBA; manually correct
        if self.filePath.endswith("dds"):
            for color in colors:
                tmp = color[0]
                color[0] = color[2]
                color[2] = tmp

        # ImageMagick decodes EXR files with premult alpha; manually correct
        if self.filePath.endswith("exr"):
            for color in colors:
                color[0] /= color[3]
                color[1] /= color[3]
                color[2] /= color[3]

        # Undo list canonicalization if we were given a single scalar coord
        if not isList:
            return colors[0]

        return colors
