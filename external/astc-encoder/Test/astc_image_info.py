#!/usr/bin/env python3
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
The ``astc_image_info`` utility provides basic image query capabilities. It is
a modal command line utility, exposing multiple available operators.

* ``info``: Query structural information about the image, such as image
      dimensions, number of color channels, and the min/max of each channel.
* ``color``: Query the stored color value at a specific pixel coordinate, and
      print the result in a variety of different formats.

Both modes allow multiple images to be specified on the command line.
"""

import argparse
import sys

from PIL import Image


def main_color(args):
    """
    Main function for the "color" mode.

    This mode prints the color at a specific pixel coordinate in each image.
    The color value is printed in a variety of color formats (decimal, HTML
    string, float).

    Args:
        args (Namespace): The parsed command line arguments.

    Returns:
        int: The process return code.
    """
    retCode = 0

    for i, image in enumerate(args.images):
        if i != 0:
            print("")

        img = Image.open(image.name)
        x = args.location[0]
        y = args.location[1]

        print(image.name)
        print("=" * len(image.name))

        if (x >= img.size[0]) or (y >= img.size[1]):
            print("- ERROR: location out-of-bounds [%ux%u]" % img.size)
            retCode = 1
        else:
            color = img.getpixel((x, y))

            # Print byte values
            print("+ Byte: %s" % str(color))

            # Print hex values
            fmtString = "+ Hex: #" + ("%02X" * len(color))
            print(fmtString % color)

            # Print float values
            parts = ["%g"] * len(color)
            parts = ", ".join(parts)
            fmtString = "+ Float: (" + parts + ")"
            print(fmtString % tuple(float(x)/255.0 for x in color))

    return retCode


def main_info(args):
    """
    Main function for the "info" mode.

    This mode prints the basic metadata of an image:

        - the overall image size.
        - the number of color channels.
        - the min/max value in each color channel.

    Args:
        args (Namespace): The parsed command line arguments.

    Returns:
        int: The process return code.
    """
    for i, image in enumerate(args.images):
        if i != 0:
            print("")

        img = Image.open(image.name)
        minmax = img.getextrema()

        print(image.name)
        print("=" * len(image.name))
        print("+ Size: %ux%u" % (img.size[0], img.size[1]))
        print("+ Channels: %s" % ("".join(img.getbands())))
        for j, channel in enumerate(img.getbands()):
            print("  + %s: %u - %u" % (channel, *minmax[j]))

    return 0


def parse_loc(value):
    """
    Command line argument parser for position arguments.

    Args:
        value (str): The command line argument string to parse. Must be of the
            form <int>x<int>", where both integers must be zero or positive.

    Returns:
        list(int, int): The parsed location.

    Raises:
        ArgumentTypeError: The value is not a valid location.
    """
    error = argparse.ArgumentTypeError("%s is an invalid location" % value)
    svalue = value.split("x")

    if len(svalue) != 2:
        raise error

    try:
        ivalue = [int(x) for x in svalue if int(x) >= 0]
    except ValueError:
        raise error

    if len(ivalue) != len(svalue):
        raise error

    return ivalue


def parse_command_line():
    """
    Parse the command line.

    Returns:
        Namespace: The parsed command line container.
    """
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(
        title="Operations")

    # Create the parser for the "pipette" command
    parserA = subparsers.add_parser(
        "color",
        help="Print color at given coordinate")

    parserA.set_defaults(func=main_color)

    parserA.add_argument(
        "location", metavar="loc", type=parse_loc,
        help="The location spec XxY")

    parserA.add_argument(
        "images", metavar="image", nargs="+", type=argparse.FileType("r"),
        help="The images to query")

    # Create the parser for the "size" command
    parserB = subparsers.add_parser(
        "info",
        help="Print image metadata info")

    parserB.set_defaults(func=main_info)

    parserB.add_argument(
        "images", metavar="image", nargs="+", type=argparse.FileType("r"),
        help="The images to query")

    # Cope with the user failing to specify any sub-command. Note on Python 3.8
    # we could use required=True on the add_subparsers call, but we cannot do
    # this on 3.6 which is our current min-spec.
    args = parser.parse_args()
    if not hasattr(args, "func"):
        parser.print_help()
        return None

    return args


def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """
    args = parse_command_line()
    if args:
        return args.func(args)

    return 0


if __name__ == "__main__":
    sys.exit(main())
