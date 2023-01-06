# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass
from .ktx_pack_uastc_flag_bits import KtxPackUastcFlagBits


@dataclass
class KtxBasisParams:
    uastc: bool = False
    verbose: bool = False
    no_sse: bool = False
    thread_count: int = 1
    compression_level: int = 0
    quality_level: int = 0
    max_endpoints: int = 0
    endpoint_rdo_threshold: int = 0
    max_selectors: int = 0
    selector_rdo_threshold: int = 0
    input_swizzle: bytes = bytes(4)
    normal_map: bool = False
    pre_swizzle: bool = False
    separate_rg_to_rgb_a: bool = False
    no_endpoint_rdo: bool = False
    no_selector_rdo: bool = False
    uastc_flags: int = KtxPackUastcFlagBits.FASTEST
    uastc_rdo: bool = False
    uastc_rdo_quality_scalar: float = 0.
    uastc_rdo_dict_size: int = 0
    uastc_rdo_max_smooth_block_error_scale: float = 10.
    uastc_rdo_max_smooth_block_std_dev: float = 18.
    uastc_rdo_dont_favor_simpler_modes: bool = False
    uastc_rdo_no_multithreading: bool = False
