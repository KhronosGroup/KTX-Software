# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from pyktx import *
from test_config import __test_images__
import unittest


class TestKtxTexture(unittest.TestCase):
    def test_kv_data(self):
        test_ktx_file = os.path.join(__test_images__, 'astc_ldr_4x4_FlightHelmet_baseColor.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.kv_data.find_value('KTXorientation'), b'rd\x00')
        self.assertEqual(texture.kv_data.copy(), {
            'KTXorientation': b'rd\x00',
            'KTXwriter': b'toktx v4.0.__default__ / libktx v4.0.__default__\x00',
            'KTXwriterScParams': b'--encode astc --astc_blk_d 4x4\x00',
        })

        for key in ['KTXorientation', 'KTXwriter', 'KTXwriterScParams']:
            texture.kv_data.delete_kv_pair(key)
        texture.kv_data.add_kv_pair('KTXwriter', b'pyktx v4.0.__default__ / libktx v4.0.__default__\x00')

        self.assertEqual(texture.kv_data.find_value('KTXwriter'),
                         b'pyktx v4.0.__default__ / libktx v4.0.__default__\x00')
