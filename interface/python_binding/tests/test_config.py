# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
import os

__test_images__ = os.environ['KTX_IMAGES_DIR'] \
    if 'KTX_IMAGES_DIR' in os.environ \
    else str((Path(__file__) / Path('../../../../tests/testimages')).resolve())
