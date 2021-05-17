# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2020 Arm Limited
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
An ASTC TestSet is comprised of a set of TestImages. Images are stored in a
structured directory layout. This structure encodes important metadata about
each image - such as color profile and data encoding - in the directory and
file names used.

TestSets are built by using reflection on a root directory to automatically
find all of the test images that comprise the set.
"""

import os

from testlib.image import TestImage


class TSetException(Exception):
    """
    Exception thrown for bad test set specification.
    """


class TestSet():
    """
    Generate a list of images that are test candidates.

    This reflection is built automatically based on a directory of images on
    disk, provided that the images follow a standard structure.

    Attributes:
        name: The name of the test set.
        tests: The list of TestImages forming the set.
    """

    def __init__(self, name, rootDir, profiles, formats, imageFilter=None):
        """
        Create a new TestSet through reflection.

        Args:
            name (str): The name of the test set.
            rootDir (str): The root directory of the test set.
            profiles (list(str)): The ASTC profiles to allow.
            formats (list(str)): The image formats to allow.
            imageFilter (str): The name of the image to include (for bug repo).

        Raises:
            TSetException: The specified TestSet could not be loaded.
        """
        self.name = name

        if not os.path.exists(rootDir) and not os.path.isdir(rootDir):
            raise TSetException("Bad test set root directory (%s)" % rootDir)

        self.tests = []

        for (dirPath, dirNames, fileNames) in os.walk(rootDir):
            for fileName in fileNames:
                # Only select image files
                fileExt = os.path.splitext(fileName)[1]
                if fileExt not in TestImage.TEST_EXTS:
                    continue

                # Create the TestImage for each file on disk
                filePath = os.path.join(dirPath, fileName)
                image = TestImage(filePath)

                # Filter out the ones we don't want to allow
                if image.colorProfile not in profiles:
                    continue

                if image.colorFormat not in formats:
                    continue

                if imageFilter and image.testFile != imageFilter:
                    continue

                self.tests.append((filePath, image))

        # Sort the TestImages so they are in a stable order
        self.tests.sort()
        self.tests = [x[1] for x in self.tests]
