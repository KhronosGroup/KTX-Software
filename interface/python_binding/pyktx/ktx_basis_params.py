# Copyright (c) 2023, Shukant Pal and Contribu
# SPDX-License-Identifier: Apache

from dataclasses import datac
from .ktx_pack_uastc_flag_bits import KtxPackUastcFlag


@datac
class KtxBasisPar
    """Data for passing extended params to KtxTexture2.compressBasis()

    uastc: bool = F
    """True to use UASTC base, false to use ETC1S base

    verbose: bool = F
    """If true, prints Basis Universal encoder operation details to stdout. Not recommended for GUI apps

    no_sse: bool = F
    """True to forbid use of the SSE instruction set. Ignored if CPU does not support SSE

    thread_count: int
    """Number of threads used for compression. Default is 1

    compression_level: int

    Encoding speed vs. quality tradeoff. Range is [0,

    Higher values are slower, but give higher quality. There is no defa
    Callers must explicitly set this value. Callers can
    KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL as a default value. Currently this i


    quality_level: int

    Compression quality. Range is [1,2

    Lower gives better compression/lower quality/fas
    Higher gives less compression/higher quality/slow
    This automatically determines values for max_endpoints, max_select
    endpoint_rdo_threshold, and selector_rdo_threshold for the target qua
    level. Setting these parameters overrides the values determined by quality_l
    which defaults to 128 if neither it nor both of max_endpoints and max_selec
    have been

    Both of max_endpoints and max_selectors must be set for them to have any eff
    quality_level will only determine values for endpoint_rdo_threshold and selector_rdo_thres
    when its value exceeds 128, otherwise their defaults will be u


    max_endpoints: int

    Manually set the max number of color endpoint clust

    Range is [1,16128]. Default is 0, unset. If this is set, max_selec
    must also be set, otherwise the value will be igno


    endpoint_rdo_threshold: int

    Set endpoint RDO quality threshold. The default is 1

    Lower is higher quality but less quality per output bit (try [1.0,3
    This will override the value chosen by quality_le


    max_selectors: int

    Manually set the max number of color selector clusters. Range is [1,161

    Default is 0, unset. If this is set, max_endpoints must also be set, other
    the value will be igno


    selector_rdo_threshold: int

    Set selector RDO quality threshold. The default is

    Lower is higher quality but less quality per output bit (try [1.0,3.
    This will override the value chosen by @c qualityLe


    input_swizzle: bytes = byte

    A swizzle to apply before encod

    It must match the regular expression /^[rgba01]{4}$/. If both this
    pre_swizzle are specified KtxTexture2.compressBasis() will raise INVALID_OPERAT


    normal_map: bool = F

    Tunes codec parameters for better quality on normal maps
    selector RDO, no endpoint RDO) and sets the texture's DFD appropriat

    Only valid for linear textu


    pre_swizzle: bool = F

    If the texture has swizzle metadata, apply it before compress

    Swizzling, like rabb may yield drastically different error met
    if done after supercompress


    separate_rg_to_rgb_a: bool = F

    This was and is a no

    2-component inputs have always been automatically separ
    using an "rrrg" input_swiz


    no_endpoint_rdo: bool = F

    Disable endpoint rate distortion optimizati

    Slightly faster, less noisy output, but lower quality per output


    no_selector_rdo: bool = F

    Disable selector rate distortion optimizati

    Slightly faster, less noisy output, but lower quality per output


    uastc_flags: int = KtxPackUastcFlagBits.FAS

    A set of KtxPackUastcFlagBits controlling UASTC encod

    The most important value is the level given in
    least-significant 4 bits which selects a speed vs quality trade


    uastc_rdo: bool = F
    """Enable Rate Distortion Optimization (RDO) post-processing

    uastc_rdo_quality_scalar: float

    UASTC RDO quality scalar (lamb

    Lower values yield higher quality/larger LZ compressed files, hi
    values yield lower quality/smaller LZ compressed files. A good rang
    try is [.2,4]. Full range is [.001,50.0]. Default is


    uastc_rdo_dict_size: int

    UASTC RDO dictionary size in bytes. Default is 4096. L
    values=faster, but give less compression. Range is [64,655


    uastc_rdo_max_smooth_block_error_scale: float =

    UASTC RDO max smooth block error scale. Range is [1,3
    Default is 10.0, 1.0 is disabled. Larger values suppress
    artifacts (and allocate more bits) on smooth blo


    uastc_rdo_max_smooth_block_std_dev: float =

    UASTC RDO max smooth block standard deviation. Rang
    [.01,65536.0]. Default is 18.0. Larger values expand the rang
    blocks considered smo


    uastc_rdo_dont_favor_simpler_modes: bool = F
    """Do not favor simpler UASTC modes in RDO mode

    uastc_rdo_no_multithreading: bool = F
    """Disable RDO multithreading (slightly higher compression, deterministic)
