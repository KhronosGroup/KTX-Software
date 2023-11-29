# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

import os

LIBKTX_INSTALL_DIR = os.getenv("LIBKTX_INSTALL_DIR")
LIBKTX_LIB_DIR = os.getenv("LIBKTX_LIB_DIR")

if os.name == 'nt':
    if LIBKTX_INSTALL_DIR is None:
        LIBKTX_INSTALL_DIR = 'C:\\Program Files\\KTX-Software'
    if LIBKTX_LIB_DIR is None:
        LIBKTX_LIB_DIR = LIBKTX_INSTALL_DIR + '\\bin'
    os.add_dll_directory(os.path.normpath(LIBKTX_LIB_DIR))

from .gl_internalformat import *
from .ktx_astc_params import *
from .ktx_basis_params import *
from .ktx_error_code import *
from .ktx_hash_list import *
from .ktx_pack_astc_block_dimension import *
from .ktx_pack_astc_encoder_mode import *
from .ktx_pack_astc_quality_levels import *
from .ktx_pack_uastc_flag_bits import *
from .ktx_supercmp_scheme import *
from .ktx_texture import *
from .ktx_texture1 import *
from .ktx_texture2 import *
from .ktx_texture_create_flag_bits import *
from .ktx_texture_create_info import *
from .ktx_texture_create_storage import *
from .ktx_transcode_flag_bits import *
from .ktx_transcode_fmt import *
from .vk_format import *
