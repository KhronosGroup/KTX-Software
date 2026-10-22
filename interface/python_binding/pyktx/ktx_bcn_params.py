# Copyright (c) 2023, Khronos Group
# SPDX-License-Identifier: Apache-2.0

import re
from dataclasses import dataclass
from typing import Union
from .ktx_bcn_compression import KtxBCnCompression
from .ktx_pack_bcn_quality_levels import KtxPackBCnQualityLevels


@dataclass
class KtxBCnParams:
    """Data for passing parameters to KtxTexture2.compress_bcn()."""

    bcn: KtxBCnCompression
    """
    BCn format to compress the uncompressed images to. Only options related to
    the provided target BCn format are used.

    Since BC7 encoding is performed using basisu's analytical encoder which
    encodes so rapidly (on average), that apart from lower VRAM consumption
    (4bpp vs. 8bpp) and better GPU texture cache efficiency, there's little need
    to use BC1 now. BC3 still has an advantage vs. BC7, because it very strongly
    separates how RGB is encoded from the alpha channel, in a predictable way.
    """

    bcn_compression_quality: Union[int, KtxPackBCnQualityLevels]
    """
    BC1 (consequently BC2 and BC3) and BC7 compression quality. Lower values
    give faster compression speed but potentially lower quality. Higher values
    give slower compression speed but potentially better quality. There is no
    default. Caller must explicitly set this value.

    For BC1, BC2, and BC3, this maps to the range [0, 19]. For BC7, this maps to
    an OR'ed set of low-level flags.
    """

    thread_count: int = 1
    """
    Number of threads used for compression and rate distortion optimization.
    Default is 1.
    """

    bcn_rdo_quality_scalar: float = 1.0
    """
    RDO quality scalar (lambda). Controls rate vs. distortion tradeoff. Lower
    values yield higher quality/larger LZ compressed files, higher values yield
    lower quality/smaller LZ compressed files. A good range to try is [0.25,8].
    Full range is [.001,50.0]. Default is 1.0.

    The post-processor tries to minimize:
    distortion * smooth_block_scale + rate * lambda
    (rate is approximate LZ bits and distortion is scaled MSE multiplied by the
    smooth block MSE weighting factor). Larger values push the post-processor
    towards optimizing more for lower rate, and smaller values more for
    distortion.

    Currently, HDR formats (i.e., BC6HU/BC6HS) are not supported.
    """

    bcn_rdo_dict_size: int = 4096
    """
    The number of bytes up to which the encoder can look back to from each block
    to find matches. The larger this value, the slower the encoder but the
    higher the quality per LZ compressed bit. Range is [64,65536].
    Default is 4096.
    """

    bcn_rdo_max_smooth_block_error_scale: float = 0
    """
    RDO max MSE scaling factor for blocks considered to be smooth/flat. A value
    of 1.0 means no smooth block error scaling which may cause very noticeable
    artifacts for smooth/flat blocks (e.g., kodim23 test image).
    @e bcnRDOMaxSmoothBlockStdDev is used to compute, for a given block, the MSE
    scale factor in the range: 1.0 (i.e., not a smooth block) up to this max MSE
    scale factor.
    
    As to why an MSE factor has to be applied to smooth/flat blocks, the MSE for
    these blocks is too low relative to the visual impact they have when they
    get distorted. The solution implemented here is to compute the max std dev.
    of any component and use a linear function of that to scale block/trial MSE.
     
    Range is [1,300]. Default is to automatically compute a decent conservative
    smooth block MSE max scaling factor.

    If this is set to 0 (default), automatically compute a decent conservative
    smooth block MSE max scaling factor. There is no single calculation/set of
    settings that work perfectly on all input textures, but the formula in the
    code works OK for most textures at low-ish lambdas (For an example of a
    difficult texture the currently formulas/settings doesn't handle so well,
    try encoding kodim03 at lambdas 1-3). Smooth block handling is tuned so
    lambdas at or near 1 look OK on textures with smooth gradients, skies, etc.
    """

    bcn_rdo_max_smooth_block_std_dev: float = 18.0
    """
    RDO max smooth/flat block standard deviation. If the std dev. of a
    block exceeds this value, then it won't be considered as a smooth
    block (i.e., the smooth block MSE scale factor will be set to 1 for
    this block). The smaller the ratio of the std dev. of this block to
    this value the more the smooth block MSE scale factor approaches
    @p bcnRDOMaxSmoothBlockErrorScale.
    Range is [.01,65536.0]. Larger values expand the range of blocks
    considered smooth. Default is 18.0.
    """

    bcn_rdo_max_allowed_rms_increase_ratio: float = 10.0
    """
    How much the RMS error of a block is allowed to increase before a trial is
    rejected. 1.0=no increase allowed, 1.05=5% increase allowed, etc.
    Range is [1.001, 100.0]. Default is 10.0.
    """

    bcn_rdo: bool = False
    """
    Enable Rate Distortion Optimization (RDO) post-processing step on
    BCn-encoded blocks to reduce entropy with Deflate/LZMA/LZHAM optimizations.
    This is primarily used to reduce size on disk by applying a further
    compression, mainly: Deflate, LZMA, or LZHAM. RDO parameters are only used
    if this is set. Setting this might result in significantly slower encoding
    time at the benefit of potentially significantly lower bit rate (i.e.,
    number of bits per encoded texel). Default is false.
    """

    bcn_rdo_no_ultrasmooth_block_handling: bool = False
    """
    Disable the detection of extremely smooth blocks and encoding them with a
    significantly higher MSE scale factor. When disabled, a per-block mask image
    is computed, filtered, then an array of per-block MSE scale factors is
    supplied to the ERT. The end result is significantly less artifacts on
    regions containing very smooth blocks (e.g., gradients, faded background,
    skies, etc.). Enabling ultrasmooth block handling hurts rate-distortion
    performance. Default is false.

    This only applies to BC1, BC3, and BC7's RGB blocks (alpha is ignored). For
    other formats, this is silently ignored.
    """

    bcn_rdo_try_one_match: bool = False
    """
    If disabled, inject up to 2 matches into each block as opposed to just one
    match. Enabling this results in faster but but noticeably lower compression.
    Default is false.
    """

    bcn_rdo_skip_zero_mse_blocks: bool = False
    """
    Skip blocks that have zero mean-squared error (MSR). Might result in faster
    compression speed but potentially lower compression. Default is false.
    """

    bcn_rdo_no_multithreading: bool = False 
    """
    Disable RDO multithreading (potentially slightly higher compression).
    Default is false.
    """
