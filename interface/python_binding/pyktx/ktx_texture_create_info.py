# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from dataclasses import dataclass
from .gl_internalformat import GlInternalformat
from typing import Optional
from .vk_format import VkFormat


@dataclass
class KtxTextureCreateInfo:
    gl_internal_format: Optional[GlInternalformat]
    base_width: int
    base_height: int
    base_depth: int

    vk_format: VkFormat = VkFormat.VK_FORMAT_UNDEFINED
    num_dimensions: int = 2
    num_levels: int = 1
    num_layers: int = 1
    num_faces: int = 1
    is_array: bool = False
    generate_mipmaps: bool = False
