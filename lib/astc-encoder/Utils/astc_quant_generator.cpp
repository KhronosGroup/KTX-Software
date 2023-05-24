// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2023 Arm Limited
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
// ----------------------------------------------------------------------------

// This is a utility tool to generate quant tables
#include <algorithm>
#include <array>
#include <bitset>
#include <set>

/**
 * @brief The ASTC quantization methods.
 *
 * Note, the values here are used directly in the encoding in the format so do not rearrange.
 */
enum quant_method
{
	QUANT_2 = 0,
	QUANT_3 = 1,
	QUANT_4 = 2,
	QUANT_5 = 3,
	QUANT_6 = 4,
	QUANT_8 = 5,
	QUANT_10 = 6,
	QUANT_12 = 7,
	QUANT_16 = 8,
	QUANT_20 = 9,
	QUANT_24 = 10,
	QUANT_32 = 11,
	QUANT_40 = 12,
	QUANT_48 = 13,
	QUANT_64 = 14,
	QUANT_80 = 15,
	QUANT_96 = 16,
	QUANT_128 = 17,
	QUANT_160 = 18,
	QUANT_192 = 19,
	QUANT_256 = 20
};

static inline unsigned int get_quant_level(quant_method method)
{
	switch (method)
	{
	case QUANT_2:   return   2;
	case QUANT_3:   return   3;
	case QUANT_4:   return   4;
	case QUANT_5:   return   5;
	case QUANT_6:   return   6;
	case QUANT_8:   return   8;
	case QUANT_10:  return  10;
	case QUANT_12:  return  12;
	case QUANT_16:  return  16;
	case QUANT_20:  return  20;
	case QUANT_24:  return  24;
	case QUANT_32:  return  32;
	case QUANT_40:  return  40;
	case QUANT_48:  return  48;
	case QUANT_64:  return  64;
	case QUANT_80:  return  80;
	case QUANT_96:  return  96;
	case QUANT_128: return 128;
	case QUANT_160: return 160;
	case QUANT_192: return 192;
	case QUANT_256: return 256;
	}

	// Unreachable - the enum is fully described
	return 0;
}

struct quant_config {
	quant_method quant;
	unsigned int bits;
	unsigned int trits;
	unsigned int quints;
	unsigned int C;
	unsigned int masks[6];
};

const std::array<quant_config, 17> quant_configs {{
	{
		QUANT_6,
		1, 1, 0,
		204,
		{
			0b000000000,
			0b000000000,
			0b000000000,
			0b000000000,
			0b000000000,
			0b000000000
		}
	}, {
		QUANT_8,
		3, 0, 0,
		0,
		{ 0 }
	}, {
		QUANT_10,
		1, 0, 1,
		113,
		{
			0b000000000,
			0b000000000,
			0b000000000,
			0b000000000,
			0b000000000,
			0b000000000
		}
	}, {
		QUANT_12,
		2, 1, 0,
		93,
		{
			0b000000000,
			0b100010110,
			0b000000000,
			0b000000000,
			0b000000000,
			0b000000000
		}
	}, {
		QUANT_16,
		4, 0, 0,
		0,
		{ 0 }
	}, {
		QUANT_20,
		2, 0, 1,
		54,
		{
			0b000000000,
			0b100001100,
			0b000000000,
			0b000000000,
			0b000000000,
			0b000000000
		}
	}, {
		QUANT_24,
		3, 1, 0,
		44,
		{
			0b000000000,
			0b010000101,
			0b100001010,
			0b000000000,
			0b000000000,
			0b000000000
		}
	}, {
		QUANT_32,
		5, 0, 0,
		0,
		{ 0 }
	},
	{
		QUANT_40,
		3, 0, 1,
		26,
		{
			0b000000000,
			0b010000010,
			0b100000101,
			0b000000000,
			0b000000000,
			0b000000000
		}
	}, {
		QUANT_48,
		4, 1, 0,
		22,
		{
			0b000000000,
			0b001000001,
			0b010000010,
			0b100000100,
			0b000000000,
			0b000000000
		}
	}, {
		QUANT_64,
		6, 0, 0,
		0,
		{ 0 }
	}, {
		QUANT_80,
		4, 0, 1,
		13,
		{
			0b000000000,
			0b001000000,
			0b010000001,
			0b100000010,
			0b000000000,
			0b000000000
		}
	}, {
		QUANT_96,
		5, 1, 0,
		11,
		{
			0b000000000,
			0b000100000,
			0b001000000,
			0b010000001,
			0b100000010,
			0b000000000
		}
	}, {
		QUANT_128,
		7, 0, 0,
		0,
		{ 0 }
	}, {
		QUANT_160,
		5, 0, 1,
		6,
		{
			0b000000000,
			0b000100000,
			0b001000000,
			0b010000000,
			0b100000001,
			0b000000000
		}
	}, {
		QUANT_192,
		6, 1, 0,
		5,
		{
			0b000000000,
			0b000010000,
			0b000100000,
			0b001000000,
			0b010000000,
			0b100000001
		}
	}, {
		QUANT_256,
		8, 0, 0,
		0,
		{ 0 }
	}
}};

void generate_unpacked_quant(
	const quant_config& config,
	std::set<unsigned int>& set
) {
	unsigned int levels = get_quant_level(config.quant);
	unsigned int emitted = 0;

	// Value has 1 trit and N bits
	if (config.trits)
	{
		for (unsigned int D = 0; D < 3; D++)
		{
			unsigned int max_bits = 1 << config.bits;
			for (unsigned int bits = 0; bits < max_bits; bits++)
			{
				unsigned int A = (bits & 1) * 0b111111111;
				unsigned int B = 0;
				unsigned int bit = bits;
				for (const auto& mask_n: config.masks)
				{
					unsigned int bit_n = bit & 1;
					bit >>= 1;
					B += bit_n * mask_n;
				}

				unsigned int T = D * config.C + B;
				T = T ^ A;
				T = (A & 0x80) | (T >> 2);
				set.insert(T);
			}
		}
	}
	// Value has 1 quint and N bits
	else if (config.quints)
	{
		for (unsigned int D = 0; D < 5; D++)
		{
			unsigned int max_bits = 1 << config.bits;
			for (unsigned int bits = 0; bits < max_bits; bits++)
			{
				unsigned int A = (bits & 1) * 0b111111111;
				unsigned int B = 0;
				unsigned int bit = bits;
				for (const auto& mask_n: config.masks)
				{
					unsigned int bit_n = bit & 1;
					bit >>= 1;
					B += bit_n * mask_n;
				}

				unsigned int T = D * config.C + B;
				T = T ^ A;
				T = (A & 0x80) | (T >> 2);
				set.insert(T);
			}
		}
	}
	// Value has N bits
	else
	{
		unsigned int max_bits = 1 << config.bits;
		for (unsigned int bits = 0; bits < max_bits; bits++)
		{
			unsigned int T = bits << (8 - config.bits);
			int bits_remaining = 8 - config.bits;

			while (bits_remaining > 0)
			{
				int shift = bits_remaining - config.bits;
				bits_remaining -= config.bits;
				if (shift > 0)
				{
					T |= bits << shift;
				}
				else
				{
					T |= bits >> -shift;
				}
			}
			set.insert(T);
		}
	}
}

void generate_unquant_to_unpacked_quant(
	const quant_config& config,
	const std::set<unsigned int>& set
) {
	for (unsigned int i = 0; i < 256; i++)
	{
		unsigned int min_dist = 256;
		unsigned int val_lo = 256;
		unsigned int val_hi = 0;

		for (const auto& val: set)
		{
			unsigned int dist = std::max(i, val) - std::min(i, val);

			if (dist < min_dist)
			{
				min_dist = dist;
				val_lo = val;
				val_hi = val;
			}
			else if (dist == min_dist)
			{
				val_lo = std::min(val_lo, val);
				val_hi = std::max(val_hi, val);
			}
		}

		if ((i % 16) == 0)
		{
			printf("\t\t");
		}

		printf("%3u, %3u", val_lo, val_hi);

		if (i != 255)
		{
			printf(", ");
		}

		if ((i % 16) == 15)
		{
			printf("\n");
		}
	}
}

int main(void)
{
	printf("const uint8_t color_unquant_to_uquant_tables[17][512] {\n");
	for (size_t i = 0; i < quant_configs.size(); i++)
	{
		const auto& config = quant_configs[i];
		std::set<unsigned int> set;

		printf("\t{ // QUANT_%u\n", get_quant_level(config.quant));
		generate_unpacked_quant(config, set);
		generate_unquant_to_unpacked_quant(config, set);
		printf("\t},\n");
	}
	printf("};\n");
	return 0;
}
