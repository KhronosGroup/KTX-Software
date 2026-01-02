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
    """Data for passing extended parameters to KtxTexture2.compress_astc()."""

    verbose: bool = False
    """If true, prints Astc encoder operation details to stdout. Not recommended for GUI apps."""

    thread_count: int = 1
    """Number of threads used for compression. Default is 1."""

    block_dimension: KtxPackAstcBlockDimension = KtxPackAstcBlockDimension.D4x4
    """Combinations of block dimensions that astcenc supports i.e. 6x6, 8x8, 6x5 etc"""

    mode: KtxPackAstcEncoderMode = KtxPackAstcEncoderMode.DEFAULT
    """Can be {ldr/hdr} from astcenc"""

    quality_level: int = KtxPackAstcQualityLevels.FASTEST
    """astcenc supports -fastest, -fast, -medium, -thorough, -exhaustive"""

    normal_map: bool = False
    """
    Tunes codec parameters for better quality on normal maps.

    In this mode normals are compressed to X,Y components
    Discarding Z component, reader will need to generate Z
    component in shaders.
    """

    perceptual: bool = False
    """
    The codec should optimize for perceptual error, instead of direct RMS error.

    This aims to improves perceived image quality, but
    typically lowers the measured PSNR score. Perceptual methods are
    currently only available for normal maps and RGB color data.
    """

    input_swizzle: bytes = bytes([0, 0, 0, 0])
    """
    A swizzle to provide as input to astcenc.

    It must match the regular expression /^[rgba01]{4}$/.
    """

    def parse_swizzle(self, swizzle: str) -> None:
        if not swizzle_regex.match(swizzle):
            raise ValueError(swizzle
                             + " does not match the appropriate format "
                             + str(swizzle_regex)
                             + " for a swizzle")

        self.input_swizzle = swizzle.encode('ascii')
