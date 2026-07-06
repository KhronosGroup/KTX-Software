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
An Image provides a wrapper around an image file on the filesystem which allows
queries to be made against that image via a Python API.

ImageMagick is used to provide some of the heavy lifting, as it supports more
file formats than PIL.
'''

import os
from pathlib import Path
import subprocess as sp

Color = tuple[float, float, float, float]

CONVERT_BINARY = ['convert']


class Image:
    '''
    Wrapper around an image on the file system.
    '''

    # No support for KTX yet because ImageMagick doesn't support it
    SUPPORTED_LDR = ['bmp', 'jpg', 'png', 'tga']
    SUPPORTED_HDR = ['exr', 'hdr']

    @classmethod
    def is_format_supported(cls, file_format: str, profile: str):
        '''
        Test if a given file format is supported by the library.

        Args:
            file_format: The target file extension, excluding the '.'.
            profile: The color profile (ldr or hdr) of the image.

        Return:
            True if the image file format is supported, False otherwise.
        '''
        assert profile in ['ldr', 'hdr']

        if profile == 'ldr':
            return file_format in cls.SUPPORTED_LDR

        return file_format in cls.SUPPORTED_HDR

    def __init__(self, file_path: Path):
        '''
        Construct a new Image wrapper.

        Args:
            file_path: The path of the image on disk.
        '''
        self.file_path = file_path

    def get_colors(self, coord: tuple[int, int]) -> Color:
        '''
        Get the image colors at the given coordinate.

        Args:
            coord: An image coordinate to sample.

        Return:
            The sampled color. Colors are returned as float values between 0.0
            and 1.0 for LDR, and float values which may exceed 1.0 for HDR.
        '''
        x = coord[0]
        y = coord[1]

        command = list(CONVERT_BINARY)
        command.append(str(self.file_path))

        # Ensure convert factors in format origin if needed
        command.append('-auto-orient')

        command += [
            '-format', '%%[pixel:p{%u,%u}]' % (x, y),
            'info:'
        ]

        if os.name == 'nt':
            command.insert(0, 'magick')

        result = sp.run(command, stdout=sp.PIPE, stderr=sp.PIPE,
                        check=True, text=True)

        raw_color = result.stdout.strip()

        # Decode ImageMagick's annoying named color outputs. Note that this
        # only handles 'known' cases triggered by our test images, we don't
        # support the entire ImageMagick named color table.
        if raw_color == 'black':
            color = [0.0, 0.0, 0.0, 1.0]

        elif raw_color == 'white':
            color = [1.0, 1.0, 1.0, 1.0]

        elif raw_color == 'red':
            color = [1.0, 0.0, 0.0, 1.0]

        elif raw_color == 'blue':
            color = [0.0, 0.0, 1.0, 1.0]

        # Decode ImageMagick's format tuples
        elif raw_color.startswith('srgba'):
            raw_color = raw_color[6:]
            raw_color = raw_color[:-1]

            channel_str = raw_color.split(',')
            channel_flt = [0.0] * 4

            for i, channel in enumerate(channel_str):
                if (i < 3) and channel.endswith('%'):
                    channel_flt[i] = float(channel[:-1]) / 100.0
                elif (i < 3) and not channel.endswith('%'):
                    channel_flt[i] = float(channel) / 255.0
                else:
                    channel_flt[i] = float(channel)

            color = channel_flt

        elif raw_color.startswith('srgb'):
            raw_color = raw_color[5:]
            raw_color = raw_color[:-1]

            channel_str = raw_color.split(',')
            channel_flt = [0.0] * 4

            for i, channel in enumerate(channel_str):
                if (i < 3) and channel.endswith('%'):
                    channel_flt[i] = float(channel[:-1]) / 100.0
                if (i < 3) and not channel.endswith('%'):
                    channel_flt[i] = float(channel) / 255.0

            channel_flt[3] = 1.0

            color = channel_flt

        elif raw_color.startswith('rgba'):
            raw_color = raw_color[5:]
            raw_color = raw_color[:-1]

            channel_str = raw_color.split(',')
            channel_flt = [0.0] * 4

            for i, channel in enumerate(channel_str):
                if (i < 3) and channel.endswith('%'):
                    channel_flt[i] = float(channel[:-1]) / 100.0
                elif (i < 3) and not channel.endswith('%'):
                    channel_flt[i] = float(channel) / 255.0
                else:
                    channel_flt[i] = float(channel)

            color = channel_flt

        elif raw_color.startswith('rgb'):
            raw_color = raw_color[4:]
            raw_color = raw_color[:-1]

            channel_str = raw_color.split(',')
            channel_flt = [0.0] * 4

            for i, channel in enumerate(channel_str):
                if (i < 3) and channel.endswith('%'):
                    channel_flt[i] = float(channel[:-1]) / 100.0
                if (i < 3) and not channel.endswith('%'):
                    channel_flt[i] = float(channel) / 255.0

            channel_flt[3] = 1.0

            color = channel_flt

        else:
            assert False, f'Unknown color format {raw_color} at {x}, {y}'

        # ImageMagick decodes DDS as BGRA not RGBA; manually correct
        if self.file_path.suffix == 'dds':
            tmp = color[0]
            color[0] = color[2]
            color[2] = tmp

        # ImageMagick decodes EXR with premultiplied alpha; manually correct
        if self.file_path.suffix == 'exr':
            color[0] /= color[3]
            color[1] /= color[3]
            color[2] /= color[3]

        return (color[0], color[1], color[2], color[3])
