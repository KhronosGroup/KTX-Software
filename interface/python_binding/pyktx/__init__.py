import os

LIBKTX_INSTALL_DIR = os.getenv('LIBKTX_INSTALL_DIR')

if os.name == 'nt':
    if LIBKTX_INSTALL_DIR == None:
        LIBKTX_INSTALL_DIR = 'C:\\Program Files\\KTX-Software'
    os.add_dll_directory(LIBKTX_INSTALL_DIR + '\\bin')

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
