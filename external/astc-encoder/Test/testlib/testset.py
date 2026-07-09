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
A TestSet is a single suite of related images used for testing, comprised of a
list of TestImage instances stored on the file system.

Images are stored on the file system in a structured layout that encodes
important metadata about each image, such as color profile and data encoding,
as part of the directory and file names used.

The directory hierarchy convention used is:

    <set_name>/<color_profile>/<file_name>

The file name convention used is:

    <color_profile>-<color_format>-<descriptive_name>[-<flags>].<extension>
'''

from pathlib import Path
from typing import Optional


class TestImageException(Exception):
    '''
    Exception thrown for bad test image specification.
    '''


class TestSetException(Exception):
    '''
    Exception thrown for bad test set specification.
    '''


class TestImage:
    '''
    Objects of this type contain metadata for a single test image on disk.

    Attributes:
        file_path: The path of the file on disk.
        file_path_out: The path of the output file on disk.
        test_format: The test format group.
        file_name: The test file name.
        color_profile: The image compression color profile.
        color_format: The image color format.
        name: The image human name.
        is_3d: True if the image is 3D, else False.
        is_alpha_scaled: True if the image wants alpha scaling, else False.

    Class Attributes:
        TEST_EXTS: Expected test image extensions.
        PROFILES: Tuple of valid color profile values.
        FORMATS: Tuple of valid color format values.
        FLAGS: Map of valid flags (key) and their meaning (value).
    '''
    TEST_EXTS = ('.jpg', '.png', '.tga', '.dds', '.hdr', '.ktx')

    PROFILES = ('ldr', 'ldrs', 'hdr')

    FORMATS = ('l', 'la', 'xy', 'rgb', 'rgba')

    FLAGS = {
        '3': '3D image',
        'a': 'Alpha scaled image'
    }

    def __init__(self, file_path: Path):
        '''
        Create a new image definition, based on a structured file path.

        Args:
            file_path: The path of the image on disk.

        Raise:
            TestImageException: The image is not found or path is malformed.
        '''
        self.file_path = file_path.resolve()
        if not self.file_path.exists():
            raise TestImageException(f'Image does not exist ({file_path})')

        self.test_set = file_path.parent.parent.name
        self.test_format = file_path.parent.name
        self.file_name = file_path.name

        # Decode the mandatory fields
        self.color_profile = self._decode_color_profile(file_path.stem)
        self.color_format = self._decode_color_format(file_path.stem)
        self.name = self._decode_name(file_path.stem)

        # Decode flags
        flags = self._decode_flags(file_path.stem)
        self.is_3d = '3' in flags
        self.is_alpha_scaled = 'a' in flags

        # Output file path, with file stored without extension
        # TODO: Move this elsewhere as it assumes the directory structure
        root_dir = Path(__file__).parent.parent
        self.file_path_out = \
            root_dir / 'TestOutput' / \
            self.test_set / self.test_format / file_path.stem

    def _decode_color_profile(self, file_name: str) -> str:
        '''
        Utility function to decode color profile from file name.

        Args:
            file_name: The base file name to parse.

        Return:
            Color profile.

        Raise:
            TestImageException: File path is malformed.
        '''
        parts = file_name.split('-')
        if len(parts) < 3:
            raise TestImageException(f'Malformed file name ({file_name})')

        profile = parts[0]
        if profile not in self.PROFILES:
            raise TestImageException(f'Unknown color profile ({profile})')

        return profile

    def _decode_color_format(self, file_name: str) -> str:
        '''
        Utility function to decode color format from file name.

        Args:
            file_name: The base file name to parse.

        Return:
            Color format.

        Raise:
            TestImageException: File path is malformed.
        '''
        parts = file_name.split('-')
        if len(parts) < 3:
            raise TestImageException(f'Malformed file name ({self.file_name})')

        cformat = parts[1]
        if cformat not in self.FORMATS:
            raise TestImageException(f'Unknown color format ({cformat})')

        return cformat

    def _decode_name(self, file_name: str) -> str:
        '''
        Utility function to decode human readable name from file name.

        Args:
            file_name: The base file name to parse.

        Return:
            Human readable name.

        Raise:
            TestImageException: File path is malformed.
        '''
        parts = file_name.split('-')
        if len(parts) < 3:
            raise TestImageException(f'Malformed file name ({self.file_name})')

        return parts[2]

    def _decode_flags(self, file_name: str) -> set[str]:
        '''
        Utility function to decode the flags from file name.

        Args:
            file_name: The base file name to parse.

        Return:
            Set of validated flags.

        Raise:
            TestImageException: File path is malformed.
        '''
        flags: set[str] = set()
        parts = file_name.split('-')

        # No flags specified
        if len(parts) < 4:
            return flags

        # ... else decode the flags field
        raw_flags = parts[3]
        for flag in raw_flags:
            if flag in flags:
                raise TestImageException(f'Duplicate flag ({flag})')

            if flag not in self.FLAGS:
                raise TestImageException(f'Unknown flag ({flag})')

            flags.add(flag)

        return flags


class TestSet:
    '''
    A suite of related images used for testing, comprised of a list of
    TestImage instances. TestImages are discovered by reading images from a
    directory in the filesystem.

    Attributes:
        name: The name of the test set.
        tests: The list of TestImages forming the set.
    '''

    def __init__(self, name: str, set_dir: Path, profiles: list[str],
                 formats: list[str], target_image: Optional[str] = None):
        '''
        Create a new TestSet using file-system discovery to find the images.

        Args:
            name: The name of the test set.
            set_dir: The root directory of the test set.
            profiles: The compressor profiles to allow.
            formats: The image formats to allow.
            target_image: The name of the image to keep (for bug repo).

        Raise:
            TestSetException: The test set could not be loaded.
        '''
        self.name = name

        if not set_dir.exists() or not set_dir.is_dir():
            raise TestSetException(f'Bad test set directory ({set_dir})')

        self.tests = []

        for (dir_path, _, file_names) in set_dir.walk():
            for file_name in file_names:
                file_path = dir_path / file_name

                # Only select image files
                ext = file_path.suffix
                if ext not in TestImage.TEST_EXTS:
                    continue

                # Create the TestImage for each file on disk
                image = TestImage(file_path)

                # Filter out the images we don't want to keep
                if image.color_profile not in profiles:
                    continue

                if image.color_format not in formats:
                    continue

                if target_image and image.file_name != target_image:
                    continue

                self.tests.append(image)

        # Sort the images so they are in a stable order
        self.tests.sort(key=lambda x: x.file_path)
