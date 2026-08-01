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
The python test runner is designed to run some basic tests against the Python
test code base.
'''

import io
import re
import sys
import unittest

import mypy
import mypy.api
import pycodestyle
import pylint
import pylint.lint
from pylint.reporters.text import TextReporter


class PythonTests(unittest.TestCase):
    '''
    Some basic Python static analysis and style checks.
    '''

    def test_pylint(self):
        '''
        Run pylint over the codebase.
        '''
        # Run Pylint
        stream = io.StringIO()
        reporter = TextReporter(stream)
        pylint.lint.Run(['./Test'], reporter, False)
        output = stream.getvalue()

        # Write the Pylint log
        with open('pylint.log', 'w', encoding='utf-8') as handle:
            handle.write(output)

        # Analyze the results
        pattern = re.compile(r'Your code has been rated at (.*?)/10')
        match = pattern.search(output)
        self.assertIsNotNone(match)
        score = float(match.group(1))

        # This target is currently low but we will increase over time
        self.assertGreaterEqual(score, 9.95, 'Found Pylint score regression')

    def test_pycodestyle(self):
        '''
        Run pycodestyle over the codebase.
        '''
        style = pycodestyle.StyleGuide()

        # Write the Pycodestyle log
        with open('pycodestyle.log', 'w', encoding='utf-8') as handle:
            # Redirect stdout to file so we can capture output
            old_stdout = sys.stdout
            sys.stdout = handle

            result = style.check_files(['./Test'])

            # Restore stdout
            sys.stdout = old_stdout

        self.assertEqual(result.total_errors, 0,
                         'Found pycodestyle warnings or errors.')

    def test_mypy(self):
        '''
        Run mypy over the codebase.
        '''
        # pylint: disable=c-extension-no-member
        result = mypy.api.run(['./Test'])

        # Write the mypy log
        if result[0]:
            with open('mypy.log', 'w', encoding='utf-8') as handle:
                handle.write(result[0])

            # Extract actual result lines from the log
            lines = result[0].splitlines()
            error_lines = []
            for line in lines:
                # Skip lines that are just status
                if line.startswith('Success'):
                    continue
                if line.startswith('Found'):
                    continue

                error_lines.append(line)

            self.assertEqual(len(error_lines), 0,
                             'Found mypy warnings or errors.')

        self.assertFalse(bool(result[1]),
                         'mypy failed to run.')


def main():
    '''
    The main function.

    Return:
        int: The process return code.
    '''
    results = unittest.main(exit=False)
    return 0 if results.result.wasSuccessful() else 1


if __name__ == '__main__':
    sys.exit(main())
