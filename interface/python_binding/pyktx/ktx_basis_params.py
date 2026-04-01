# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass
from .ktx_basis_codec import KtxBasisCodec
from .ktx_pack_uastc_flag_bits import KtxPackUastcFlagBits

@dataclass
class KtxBasisParams:
    """Struct for passing extended params to KtxTexture2.compressBasis()."""

    codec: int = KtxBasisCodec.ETC1S
    """Basis Universal codec to use. Default is KtxBasisCodec.ETC1S/BasisLZ."""

    verbose: bool = False
    """If true, prints Basis Universal encoder operation details to stdout. Not recommended for GUI apps."""

    no_sse: bool = False
    """True to forbid use of the SSE instruction set. Ignored if CPU does not support SSE."""

    thread_count: int = 1
    """Number of threads used for compression. Default is 1."""

    # Here and in the descriptions of all other codec specific options we abuse the
    # fact that ':'in a docstring is used to indicate the preceding text is a
    # type so that the target codec is clearly flagged on a separate "Type: " line.
    etc1s_compression_level: int = 0
    """
    ETC1S/BasisLZ: Encoding speed vs. quality tradeoff for etc1s. Range is [0,5].

    Higher values are slower, but give higher quality. There is no default.
    Callers must explicitly set this value. Callers can use
    KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL as a default value. Currently this is 2.
    """

    quality_level: int = 0
    """
    ETC1S/BasisLZ and UASTC LDR 4x4: Compression quality. Range is [1,255].

    Lower gives better compression/lower quality/faster.
    Higher gives less compression/higher quality/slower.
    This automatically determines values for max_endpoints, max_selectors,
    endpoint_rdo_threshold, and selector_rdo_threshold for the target quality
    level. Setting these parameters overrides the values determined by quality_level
    which defaults to 128 if neither it nor both of max_endpoints and max_selectors
    have been set.

    Both of max_endpoints and max_selectors must be set for them to have any effect.
    quality_level will only determine values for endpoint_rdo_threshold and selector_rdo_threshold
    when its value exceeds 128, otherwise their defaults will be used.
    """

    max_endpoints: int = 0
    """
    ETC1S/BasisLZ: Manually set the max number of color endpoint clusters.

    Range is [1,16128]. Default is 0, unset. If this is set, max_selectors
    must also be set, otherwise the value will be ignored.
    """

    endpoint_rdo_threshold: int = 0
    """
    ETC1S/BasisLZ: Set endpoint RDO quality threshold. The default is 1.25.

    Lower is higher quality but less quality per output bit (try [1.0,3.0].
    This will override the value chosen by quality_level.
    """

    max_selectors: int = 0
    """
    ETC1S/BasisLZ: Manually set the max number of color selector clusters. Range is [1,16128].

    Default is 0, unset. If this is set, max_endpoints must also be set, otherwise
    the value will be ignored.
    """

    selector_rdo_threshold: int = 0
    """
    ETC1S/BasisLZ:  Set selector RDO quality threshold. The default is 1.5.

    Lower is higher quality but less quality per output bit (try [1.0,3.0]).
    This will override the value chosen by @c qualityLevel.
    """

    input_swizzle: bytes = bytes(4)
    """
    A swizzle to apply before encoding.

    It must match the regular expression /^[rgba01]{4}$/. If both this and
    pre_swizzle are specified KtxTexture2.compressBasis() will raise INVALID_OPERATION.
    """

    normal_map: bool = False
    """
    Tunes codec parameters for better quality on normal maps (no
    selector RDO, no endpoint RDO) and sets the texture's DFD appropriately.

    Only valid for linear textures.
    """

    pre_swizzle: bool = False
    """
    If the texture has swizzle metadata, apply it before compressing.

    Swizzling, like rabb may yield drastically different error metrics
    if done after supercompression.
    """

    separate_rg_to_rgb_a: bool = False
    """
    This was and is a no-op.

    2-component inputs have always been automatically separated
    using an "rrrg" input_swizzle.
    """

    no_endpoint_rdo: bool = False
    """
    ETC1S/BasisLZ: Disable endpoint rate distortion optimizations.

    Slightly faster, less noisy output, but lower quality per output bit.
    """

    no_selector_rdo: bool = False
    """
    ETC1S/BasisLZ: Disable selector rate distortion optimizations.

    Slightly faster, less noisy output, but lower quality per output bit.
    """

    uastc_flags: int = KtxPackUastcFlagBits.FASTEST
    """
    UASTC LDR 4x4: A set of KtxPackUastcFlagBits controlling UASTC encoding.

    The most important value is the level given in the
    least-significant 4 bits which selects a speed vs quality tradeoff.
    """

    uastc_rdo: bool = False
    """UASTC LDR 4x4: Enable Rate Distortion Optimization (RDO) post-processing."""

    uastc_rdo_quality_scalar: float = 0.
    """
    UASTC LDR 4x4: RDO quality scalar (lambda).

    Lower values yield higher quality/larger LZ compressed files, higher
    values yield lower quality/smaller LZ compressed files. A good range to
    try is [.2,4]. Full range is [.001,50.0]. Default is 1.0.
    """

    uastc_rdo_dict_size: int = 0
    """
    UASTC LDR 4x4: RDO dictionary size in bytes. Default is 4096. Lower
    values=faster, but give less compression. Range is [64,65536].
    """

    uastc_rdo_max_smooth_block_error_scale: float = 10.
    """
    UASTC LDR 4x4: RDO max smooth block error scale. Range is [1,300].
    Default is 10.0, 1.0 is disabled. Larger values suppress more
    artifacts (and allocate more bits) on smooth blocks.
    """

    uastc_rdo_max_smooth_block_std_dev: float = 18.
    """
    UASTC LDR 4x4: RDO max smooth block standard deviation. Range is
    [.01,65536.0]. Default is 18.0. Larger values expand the range of
    blocks considered smooth.
    """

    uastc_rdo_dont_favor_simpler_modes: bool = False
    """UASTC LDR 4x4: Do not favor simpler UASTC modes in RDO mode."""

    uastc_rdo_no_multithreading: bool = False
    """UASTC LDR 4x4: Disable RDO multithreading (slightly higher compression, deterministic)."""

    # UASTC HDR params

    uastc_hdr_quality: int = 1
    """
    Valid range is [0,4] - higher=slower but higher quality. Default=1.
    Level 0=fastest/lowest quality, 3=highest practical setting, 4=exhaustive
    """

    uastc_hdr_uber_mode: bool = False
    """
    UASTC HDR 4x4: Allow the UASTC HDR 4x4 encoder to try varying the CEM 11
    selectors more for slightly higher quality (slower). This may negatively impact BC6H quality, however.
    """

    uastc_hdr_ultra_quant: bool = False
    """UASTC HDR 4x4: Try to find better quantized CEM 7/11 endpoint values (slower)."""

    uastc_hdr_favor_astc: bool = False
    """
    UASTC HDR 4x4: By default the UASTC HDR 4x4 encoder tries to strike a balance
    or even slightly favor BC6H quality. If this option is specified, ASTC HDR 4x4 quality is favored instead.
    """

    rec_2020: bool = False
    """
    UASTC HDR 6x6i specific option: The input image's gamut is Rec. 2020 vs. the
    default Rec. 709 - for accurate colorspace error calculations.
    """

    uastc_hdr_lambda: float = 0.
    """
    UASTC HDR 6x6i specific option: Enables rate distortion optimization (RDO).
    The higher this value, the lower the quality, but the smaller the file size.
    Try 100-20000, or higher values on some images.
    """

    uastc_hdr_level: int = 2
    """
    UASTC HDR 6x6i specific option: Controls the 6x6 HDR intermediate mode encoder
    performance vs. max quality tradeoff. X may range from [0,12]. Default level is 2.
    """
