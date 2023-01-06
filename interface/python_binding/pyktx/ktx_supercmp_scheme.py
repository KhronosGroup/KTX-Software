# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxSupercmpScheme(IntEnum):
    NONE = 0
    BASIS_LZ = 1
    ZSTD = 2
