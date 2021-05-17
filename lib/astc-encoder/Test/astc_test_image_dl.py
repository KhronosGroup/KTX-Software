#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# -----------------------------------------------------------------------------
# Copyright 2019-2020 Arm Limited
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
The ``astc_test_image_dl`` utility provides a means to programatically download
test images that are available online, avoiding the need to duplicate them in
the git repository.
"""


import os
import sys
import urllib.request

from PIL import Image


TEST_IMAGE_DIR = os.path.join("Test", "Images")


def download(testSet, index, srcUrl, dstPath):
    """
    Download a single image.

    Args:
        testSet (str): The test set name.
        index (int): The download index.
        srcUrl (str): The download URL.
        dstPath (str): The destination path.
    """
    dirName = os.path.dirname(dstPath)
    if not os.path.exists(dirName):
        os.makedirs(dirName)

    # Skip downloads if the file already exists
    if not os.path.exists(dstPath):
        print("%s image %u: Downloading" % (testSet, index))
        urllib.request.urlretrieve(srcUrl, dstPath)
    else:
        print("%s image %u: Skipping" % (testSet, index))


def make_landscape(imgPath):
    """
    Make an image on disk landscape aspect (edit in place)

    Args:
        imgPath: The pth of the image on disk.
    """
    img = Image.open(imgPath)
    if img.size[0] < img.size[1]:
        img = img.rotate(90, expand=True)
        img.save(imgPath)


def make_mixed_image(imgPathA, imgPathB, dstPath):
    """
    Make image consisting of RGB from A's RGB, and alpha from B's luminance.

    Args:
        imgPathA: The path of input A on disk.
        imgPathB: The path of input B on disk.
        dstPath: The path of the destination.
    """
    imgA = Image.open(imgPathA)
    imgB = Image.open(imgPathB).convert("L")

    imgA.putalpha(imgB)

    dirs = os.path.dirname(dstPath)
    if not os.path.exists(dirs):
        os.makedirs(dirs)

    imgA.save(dstPath)


def make_montage(imageDir, dstPath):
    """
    Make a single mosaic montage consisting of all of the Kodak images.

    Args:
        imgDir: The directory path of the Kodak images on disk.
        dstPth: The file path of the resulting montage.
    """
    cols = 6
    rows = 4

    width = 768
    height = 512

    images = os.listdir(imageDir)
    images.sort()

    montage = Image.new('RGB', (width * cols, height * rows))

    for i, src in enumerate(images):
        im = Image.open(os.path.join(imageDir, src))
        col = i % cols
        row = i // cols
        montage.paste(im, (width * col, height * row))

    dirs = os.path.dirname(dstPath)
    if not os.path.exists(dirs):
        os.makedirs(dirs)

    montage.save(dstPath)


def retrieve_kodak_set():
    """
    Download the public domain Kodak image set.

    To make test set mosaics easier to build we rotate images to make
    everything landscape.
    """
    testSet = "Kodak"

    # Download the original RGB images
    for i in range(1, 25):
        fle = "ldr-rgb-kodak%02u.png" % i
        dst = os.path.join(TEST_IMAGE_DIR, "Kodak", "LDR-RGB", fle)
        src = "http://r0k.us/graphics/kodak/kodak/kodim%02u.png" % i
        download(testSet, i, src, dst)

        # Canonicalize image aspect
        make_landscape(dst)

    # Make some correlated alpha RGBA images
    fle = "ldr-rgb-kodak%02u.png"  # Expand later
    pattern = os.path.join(TEST_IMAGE_DIR, "Kodak", "LDR-RGB", fle)

    for i in (22, 23):
        imgA = pattern % i
        fle = "ldr-rgba-kodak%02u+ca.png" % i
        dst = os.path.join(TEST_IMAGE_DIR, "KodakSim", "LDR-RGBA", fle)
        make_mixed_image(imgA, imgA, dst)

    # Make some non-correlated alpha RGBA images
    for i, j in ((22, 24), (23, 20)):
        imgA = pattern % i
        imgB = pattern % j
        fle = "ldr-rgba-kodak%02u+%02u+nca.png" % (i, j)
        dst = os.path.join(TEST_IMAGE_DIR, "KodakSim", "LDR-RGBA", fle)
        make_mixed_image(imgA, imgB, dst)

    # Make a large montage
    srcDir = os.path.join(TEST_IMAGE_DIR, "Kodak", "LDR-RGB")
    fle = "ldr-rgb-montage.png"
    dst = os.path.join(TEST_IMAGE_DIR, "KodakMnt", "LDR-RGB", fle)
    make_montage(srcDir, dst)


def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """
    retrieve_kodak_set()
    return 0


if __name__ == "__main__":
    sys.exit(main())
