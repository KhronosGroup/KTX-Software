# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

import os

for root, directories, files in os.walk('pyktx'):
    for file in files:
        if (file.endswith('.o') or
                file.endswith('.obj') or
                file.endswith('.lib') or
                file.endswith('.so') or
                file.endswith('.dylib') or
                file.endswith('.dll') or
                file.startswith('native')):
            path = os.path.join('pyktx', file)
            print(f"Deleting {path}")
            os.remove(path)
