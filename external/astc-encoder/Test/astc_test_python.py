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
The python test runner is designed to run some basic tests against the Python
test code base.
"""

import re
import sys
import unittest

import pycodestyle
import pylint.epylint as lint


class PythonTests(unittest.TestCase):
    """
    Some basic Python static analysis and style checks.
    """

    def test_pylint(self):
        """
        Run pylint over the codebase.
        """
        pylintOut, _ = lint.py_run("./Test", True)
        pattern = re.compile(r"Your code has been rated at (.*?)/10")
        match = pattern.search(pylintOut.getvalue())
        self.assertIsNotNone(match)
        score = float(match.group(1))
        self.assertGreaterEqual(score, 9.8)

        with open("pylint.log", "w") as fileHandle:
            fileHandle.write(pylintOut.getvalue())

    def test_pycodestyle(self):
        """
        Test that we conform to PEP-8.
        """
        style = pycodestyle.StyleGuide()
        result = style.check_files(["./Test"])
        self.assertEqual(result.total_errors, 0,
                         "Found code style errors (and warnings).")


def main():
    """
    The main function.

    Returns:
        int: The process return code.
    """
    results = unittest.main(exit=False)
    return 0 if results.result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(main())
