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
A collection of useful utility functions that are not module specific.
"""

import os


def path_splitall(path):
    """
    Utility function to split a relative path into its component pieces.

    Args:
        path(str): The relative path to split.

    Returns:
        list(str): An array of path parts.
    """
    # Sanity check we have a relative path on Windows
    assert ":" not in path

    parts = []
    while path:
        head, tail = os.path.split(path)
        path = head
        parts.insert(0, tail)

    return parts
