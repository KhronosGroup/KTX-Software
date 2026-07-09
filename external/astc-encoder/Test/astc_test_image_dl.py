#!/usr/bin/env python3
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
This script is a utility to download test images that are available online,
avoiding the need to duplicate them in the repository.
'''

from pathlib import Path
import sys
import urllib.request

from PIL import Image


TEST_IMAGE_DIR = Path('Test/Images')


def download(test_set: str, index: int, src_url: str, dst_path: Path) -> None:
    '''
    Download a single image.

    Args:
        test_set: The test set name.
        index: The download image index.
        src_url: The download URL.
        dst_path: The destination file path.
    '''
    # Skip downloads if the file already exists
    if dst_path.exists():
        print(f'{test_set} image {index}: Skipping')
        return

    dir_name = dst_path.parent
    dir_name.mkdir(parents=True, exist_ok=True)

    print(f'{test_set} image {index}: Downloading')
    urllib.request.urlretrieve(src_url, dst_path)


def make_landscape(file_path: Path) -> None:
    '''
    Modify an existing image on disk to make it landscape aspect.

    Args:
        file_path: The path of the image on disk.
    '''
    image = Image.open(file_path)
    if image.size[0] < image.size[1]:
        rotated_image = image.rotate(90, expand=True)
        rotated_image.save(file_path)


def make_mixed_image(srca_path: Path, srcb_path: Path, dst_path: Path) -> None:
    '''
    Make image consisting of RGB from A, and alpha from B.

    Args:
        srca_path: The path of input A on disk.
        srcb_path: The path of input B on disk.
        dst_path: The path of the destination.
    '''
    dir_name = dst_path.parent
    dir_name.mkdir(parents=True, exist_ok=True)

    image_rgb = Image.open(srca_path)
    image_alpha = Image.open(srcb_path).convert('L')

    image_rgb.putalpha(image_alpha)
    image_rgb.save(dst_path)


def make_montage(src_dir_path: Path, dst_path: Path):
    '''
    Make a single mosaic montage consisting of all of the Kodak images.

    Args:
        src_dir_path: The directory path of the Kodak images on disk.
        dst_path: The file path of the resulting montage.
    '''
    cols = 6
    rows = 4

    width = 768
    height = 512

    images = list(src_dir_path.glob('*.png'))
    images.sort()

    montage = Image.new('RGB', (width * cols, height * rows))

    for i, src_path in enumerate(images):
        im = Image.open(src_path)
        col = i % cols
        row = i // cols
        montage.paste(im, (width * col, height * row))

    dir_name = dst_path.parent
    dir_name.mkdir(parents=True, exist_ok=True)

    montage.save(dst_path)


def retrieve_kodak_set():
    '''
    Download the public domain Kodak image set.

    To make test set mosaics easier to build we rotate images to make
    everything landscape.
    '''
    test_set = 'Kodak'

    # Download the original RGB images
    for i in range(1, 25):
        src_url = f'http://r0k.us/graphics/kodak/kodak/kodim{i:02}.png'

        dst_file = f'ldr-rgb-kodak{i:02}.png'
        dst_path = TEST_IMAGE_DIR / 'Kodak' / 'LDR-RGB' / dst_file
        download(test_set, i, src_url, dst_path)

        # Canonicalize image aspect
        make_landscape(dst_path)

    # Make some correlated alpha RGBA images
    src_dir = TEST_IMAGE_DIR / 'Kodak' / 'LDR-RGB'
    dst_dir = TEST_IMAGE_DIR / 'KodakSim' / 'LDR-RGBA'

    for i in (22, 23):
        src_path = src_dir / f'ldr-rgb-kodak{i:02}.png'
        dst_path = dst_dir / f'ldr-rgba-kodak{i:02}+ca.png'
        make_mixed_image(src_path, src_path, dst_path)

    # Make some non-correlated alpha RGBA images
    for i, j in ((22, 24), (23, 20)):
        srca_path = src_dir / f'ldr-rgb-kodak{i:02}.png'
        srcb_path = src_dir / f'ldr-rgb-kodak{j:02}.png'

        dst_path = dst_dir / f'ldr-rgba-kodak{i:02}+{j:02}+nca.png'
        make_mixed_image(srca_path, srcb_path, dst_path)

    # Make a large montage
    dst_path = TEST_IMAGE_DIR / 'KodakMnt' / 'LDR-RGBA' / 'ldr-rgb-montage.png'
    make_montage(src_dir, dst_path)


def main() -> int:
    '''
    The main function.

    Return:
        The process return code.
    '''
    retrieve_kodak_set()
    return 0


if __name__ == '__main__':
    sys.exit(main())
