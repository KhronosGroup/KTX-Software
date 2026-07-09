// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2025-2026 Arm Limited
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

/**
 * @brief Unit tests for the vectorized SIMD functionality.
 */

#include <limits>

#include "gtest/gtest.h"

#include "../astcenc.h"

namespace astcenc
{

/** @brief Input overflow tests for xy * z. */
TEST(compress, overflow_in_z)
{
	astcenc_error status;
	astcenc_config config;
	astcenc_context* context;

	static const astcenc_swizzle swizzle {
		ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
	};

	astcenc_config_init(ASTCENC_PRF_LDR, 4, 4, 1, ASTCENC_PRE_MEDIUM, 0, &config);
	status = astcenc_context_alloc(&config, 1, &context, nullptr);
	EXPECT_EQ(status, ASTCENC_SUCCESS);

	// Arrays are too short, but should never be touched
	uint8_t data[1];
	uint8_t input[1];

	astcenc_image image;
	// x * y always fits in 64-bit size_t, but xy * z will overflow
	image.dim_x = 0xFFFFFFFFu;
	image.dim_y = 0xFFFFFFFFu;
	image.dim_z = 0xFFFFFFFFu;
	image.data_type = ASTCENC_TYPE_U8;
	uint8_t* slices = input;
	image.data = reinterpret_cast<void**>(&slices);

	status = astcenc_compress_image(context, &image, &swizzle, data, -1, 0);
	EXPECT_EQ(status, ASTCENC_ERR_BAD_PARAM);

	astcenc_context_free(context);
}

/** @brief Input overflow tests for xyz * 16. */
TEST(compress, overflow_in_16)
{
	astcenc_error status;
	astcenc_config config;
	astcenc_context* context;

	static const astcenc_swizzle swizzle {
		ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
	};

	astcenc_config_init(ASTCENC_PRF_LDR, 4, 4, 1, ASTCENC_PRE_MEDIUM, 0, &config);
	status = astcenc_context_alloc(&config, 1, &context, nullptr);
	EXPECT_EQ(status, ASTCENC_SUCCESS);

	// Arrays are too short, but should never be touched
	uint8_t data[1];
	uint8_t input[1];

	astcenc_image image;
	// xyz (in blocks) always fits in 64-bit size_t, but xyz * 16 will overflow
	image.dim_x = 0x80000000u;
	image.dim_y = 0x80000000u;
	image.dim_z = 0x00000010u;
	image.data_type = ASTCENC_TYPE_U8;
	uint8_t* slices = input;
	image.data = reinterpret_cast<void**>(&slices);

	status = astcenc_compress_image(context, &image, &swizzle, data, -1, 0);
	EXPECT_EQ(status, ASTCENC_ERR_BAD_PARAM);

	astcenc_context_free(context);
}


/** @brief Input data buffer overrun tests. */
TEST(compress, data_buffer_exceeded)
{
	astcenc_error status;
	astcenc_config config;
	astcenc_context* context;

	static const astcenc_swizzle swizzle {
		ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
	};

	astcenc_config_init(ASTCENC_PRF_LDR, 4, 4, 1, ASTCENC_PRE_MEDIUM, 0, &config);
	status = astcenc_context_alloc(&config, 1, &context, nullptr);
	EXPECT_EQ(status, ASTCENC_SUCCESS);

	// Arrays are too short, but should never be touched
	uint8_t data[1];
	uint8_t input[1];

	astcenc_image image;
	image.dim_x = 0x4u;
	image.dim_y = 0x4u;
	image.dim_z = 0x1u;
	image.data_type = ASTCENC_TYPE_U8;
	uint8_t* slices = input;
	image.data = reinterpret_cast<void**>(&slices);

	// Data size is 1 byte too short so this should error
	status = astcenc_compress_image(context, &image, &swizzle, data, 15, 0);
	EXPECT_EQ(status, ASTCENC_ERR_OUT_OF_MEM);

	astcenc_context_free(context);
}

/** @brief Helper to run a compression on a single 4x4 block */
static void run_positive_test(
	astcenc_profile profile,
	float* input
) {
	astcenc_error status;
	astcenc_config config;
	astcenc_context* context;

	static const astcenc_swizzle swizzle {
		ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
	};

	astcenc_config_init(profile, 4, 4, 1, ASTCENC_PRE_MEDIUM, 0, &config);
	status = astcenc_context_alloc(&config, 1, &context, nullptr);
	EXPECT_EQ(status, ASTCENC_SUCCESS);

	// Single 16 byte block output data
	uint8_t data[16];

	astcenc_image image;
	image.dim_x = 4u;
	image.dim_y = 4u;
	image.dim_z = 1u;
	image.data_type = ASTCENC_TYPE_F32;
	float* slices = input;
	image.data = reinterpret_cast<void**>(&slices);

	// Should not error or trip any UBSAN errors
	status = astcenc_compress_image(context, &image, &swizzle, data, 16, 0);
	EXPECT_EQ(status, ASTCENC_SUCCESS);

	astcenc_context_free(context);
}

/** @brief Input data contains negative infinities. */
TEST(compress, data_neg_inf_ldr)
{
	// Cycle through a single bad value in each color channel
	for(int offset = 0; offset < 4; offset++)
	{
		// 4x4 block input data
		std::vector<float> input(4 * 4 * 4, 0.5f);
		input[0 + offset] = -std::numeric_limits<float>::infinity();

		// Make data more complex to hit more compressor paths
		input[4 + offset] = 0.75f;
		input[8 + offset] = 0.35f;
		input[12 + offset] = 0.0f;

		run_positive_test(ASTCENC_PRF_LDR, input.data());
	}
}

/** @brief Input data contains negative infinities. */
TEST(compress, data_neg_inf_hdr)
{
	// Cycle through a single bad value in each color channel
	for(int offset = 0; offset < 4; offset++)
	{
		// 4x4 block input data
		std::vector<float> input(4 * 4 * 4, 0.5f);
		input[0 + offset] = -std::numeric_limits<float>::infinity();

		run_positive_test(ASTCENC_PRF_HDR, input.data());
	}
}

/** @brief Input data contains negative infinities. */
TEST(compress, data_neg_inf_hdr_ldra)
{
	// Cycle through a single bad value in each color channel
	for(int offset = 0; offset < 4; offset++)
	{
		// 4x4 block input data
		std::vector<float> input(4 * 4 * 4, 0.5f);
		input[0 + offset] = -std::numeric_limits<float>::infinity();

		run_positive_test(ASTCENC_PRF_HDR_RGB_LDR_A, input.data());
	}
}

/** @brief Input data contains positive infinities. */
TEST(compress, data_pos_inf_ldr)
{
	// Cycle through a single bad value in each color channel
	for(int offset = 0; offset < 4; offset++)
	{
		// 4x4 block input data
		std::vector<float> input(4 * 4 * 4, 0.5f);
		input[0 + offset] = std::numeric_limits<float>::infinity();

		run_positive_test(ASTCENC_PRF_LDR, input.data());
	}
}

/** @brief Input data contains positive infinities. */
TEST(compress, data_pos_inf_hdr)
{
	// Cycle through a single bad value in each color channel
	for(int offset = 0; offset < 4; offset++)
	{
		// 4x4 block input data
		std::vector<float> input(4 * 4 * 4, 0.5f);
		input[0 + offset] = std::numeric_limits<float>::infinity();

		run_positive_test(ASTCENC_PRF_HDR, input.data());
	}
}

/** @brief Input data contains positive infinities. */
TEST(compress, data_pos_inf_hdr_ldra)
{
	// Cycle through a single bad value in each color channel
	for(int offset = 0; offset < 4; offset++)
	{
		// 4x4 block input data
		std::vector<float> input(4 * 4 * 4, 0.5f);
		input[0 + offset] = std::numeric_limits<float>::infinity();

		run_positive_test(ASTCENC_PRF_HDR_RGB_LDR_A, input.data());
	}
}


/** @brief Input data contains NaN. */
TEST(compress, data_nan_ldr)
{
	// Cycle through a single bad value in each color channel
	for(int offset = 0; offset < 4; offset++)
	{
		// 4x4 block input data
		std::vector<float> input(4 * 4 * 4, 0.5f);
		input[0 + offset] = std::numeric_limits<float>::quiet_NaN();

		run_positive_test(ASTCENC_PRF_LDR, input.data());
	}
}

/** @brief Input data contains NaN. */
TEST(compress, data_nan_hdr)
{
	// Cycle through a single bad value in each color channel
	for(int offset = 0; offset < 4; offset++)
	{
		// 4x4 block input data
		std::vector<float> input(4 * 4 * 4, 0.5f);
		input[0 + offset] = std::numeric_limits<float>::quiet_NaN();

		run_positive_test(ASTCENC_PRF_HDR, input.data());
	}
}

/** @brief Input data contains NaN. */
TEST(compress, data_nan_hdr_ldra)
{
	// Cycle through a single bad value in each color channel
	for(int offset = 0; offset < 4; offset++)
	{
		// 4x4 block input data
		std::vector<float> input(4 * 4 * 4, 0.5f);
		input[0 + offset] = std::numeric_limits<float>::quiet_NaN();

		run_positive_test(ASTCENC_PRF_HDR_RGB_LDR_A, input.data());
	}
}

}
