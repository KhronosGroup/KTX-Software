# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass
from .gl_internalformat import GlInternalformat
from typing import Optional
from .vk_format import VkFormat


@dataclass
class KtxTextureCreateInfo:
    """Data for passing texture information to KtxTexture1.create() and KtxTexture2.create()."""

    gl_internal_format: Optional[GlInternalformat]
    """Internal format for the texture, e.g., GlInteralformat.RGB8. Ignored when creating a KtxTexture2."""

    base_width: int
    """Width of the base level of the texture."""

    base_height: int
    """Height of the base level of the texture."""

    base_depth: int
    """Depth of the base level of the texture."""

    vk_format: VkFormat = VkFormat.VK_FORMAT_UNDEFINED
    """VkFormat for texture. Ignored when creating a KtxTexture1."""

    num_dimensions: int = 2
    """Number of dimensions in the texture, 1, 2 or 3."""

    num_levels: int = 1
    """Number of mip levels in the texture. Should be 1 if generateMipmaps is true."""

    num_layers: int = 1
    """Number of array layers in the texture."""

    num_faces: int = 1
    """Number of faces: 6 for cube maps, 1 otherwise."""

    is_array: bool = False
    """Set to true if the texture is to be an array texture. Means OpenGL will use a GL_TEXTURE_*_ARRAY target."""

    generate_mipmaps: bool = False
    """Set to true if mipmaps should be generated for the texture when loading into a 3D API."""
