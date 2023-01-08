# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

import re
from dataclasses import dataclass
from .ktx_pack_astc_block_dimension import KtxPackAstcBlockDimension
from .ktx_pack_astc_encoder_mode import KtxPackAstcEncoderMode
from .ktx_pack_astc_quality_levels import KtxPackAstcQualityLevels

swizzle_regex = re.compile('^[rgba01]{4}$')


@dataclass
class KtxAstcParams:
    verbose: bool = False
    thread_count: int = 1
    block_dimension: KtxPackAstcBlockDimension = KtxPackAstcBlockDimension.D4x4
    mode: KtxPackAstcEncoderMode = KtxPackAstcEncoderMode.DEFAULT
    quality_level: int = KtxPackAstcQualityLevels.FASTEST
    normal_map: bool = False
    perceptual: bool = False
    input_swizzle: bytes = bytes([0, 0, 0, 0])

    def parse_swizzle(self, swizzle: str) -> None:
        if not swizzle_regex.match(swizzle):
            raise ValueError(swizzle
                             + " does not match the appropriate format "
                             + str(swizzle_regex)
                             + " for a swizzle")

        self.input_swizzle = swizzle.encode('ascii')
