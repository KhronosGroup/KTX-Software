// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2021 Arm Limited
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

// This is a utility tool to test blend modes.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "astcenc_mathlib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/**
 * @brief Linearize an sRGB value.
 *
 * @return The linearized value.
 */
static float srgb_to_linear(
	float a
) {
	if (a <= 0.04045f)
	{
		return a * (1.0f / 12.92f);
	}

	return powf((a + 0.055f) * (1.0f / 1.055f), 2.4f);
}

/**
 * @brief sRGB gamma-encode a linear value.
 *
 * @return The gamma encoded value.
 */
static float linear_to_srgb(
	float a
) {
	if (a <= 0.0031308f)
	{
		return a * 12.92f;
	}

	return 1.055f * powf(a, 1.0f / 2.4f) - 0.055f;
}

int main(int argc, char **argv)
{
	// Parse command line
	if (argc != 6)
	{
		printf("Usage: astc_blend_test <source> <dest> <format> <blend_mode> <filter>\n");
		exit(1);
	}

	const char* src_file = argv[1];
	const char* dst_file = argv[2];

	bool use_linear = false;
	if (!strcmp(argv[3], "linear"))
	{
		use_linear = true;
	}
	else if (!strcmp(argv[3], "srgb"))
	{
		use_linear = false;
	}
	else
	{
		printf("<format> must be either 'linear' or 'srgb'\n");
		exit(1);
	}

	bool use_post_blend = false;
	if (!strcmp(argv[4], "post"))
	{
		use_post_blend = true;
	}
	else if (!strcmp(argv[4], "pre"))
	{
		use_post_blend = false;
	}
	else
	{
		printf("<blend_mode> must be either 'post' or 'pre'\n");
		exit(1);
	}

	bool use_filter = false;
	if (!strcmp(argv[5], "on"))
	{
		use_filter = true;
	}
	else if (!strcmp(argv[5], "off"))
	{
		use_filter == false;
	}
	else
	{
		printf("<filter> must be either 'on' or 'off'\n");
		exit(1);
	}

	// Load the input image
	int dim_x;
	int dim_y;
	const uint8_t* data_in = stbi_load(src_file, &dim_x, &dim_y, nullptr, 4);
	if (!data_in)
	{
		printf("ERROR: Failed to load input image.\n");
		exit(1);
	}

	// Allocate the output image
	uint8_t* data_out = (uint8_t*)malloc(4 * dim_y * dim_x);
	if (!data_out)
	{
		printf("ERROR: Failed to allocate output image.\n");
		exit(1);
	}

	// For each pixel apply RGBM encoding
	if (!use_filter)
	{
		for (int y = 0; y < dim_y; y++)
		{
			const uint8_t* row_in = data_in + (4 * dim_x * y);
			uint8_t* row_out = data_out + (4 * dim_x * y);

			for (int x = 0; x < dim_x; x++)
			{
				const uint8_t* pixel_in = row_in + 4 * x;
				uint8_t* pixel_out = row_out + 4 * x;

				float r_src = static_cast<float>(pixel_in[0]) / 255.0f;
				float g_src = static_cast<float>(pixel_in[1]) / 255.0f;
				float b_src = static_cast<float>(pixel_in[2]) / 255.0f;
				float a_src = static_cast<float>(pixel_in[3]) / 255.0f;

				if (use_linear == false)
				{
					r_src = srgb_to_linear(r_src);
					g_src = srgb_to_linear(g_src);
					b_src = srgb_to_linear(b_src);
				}

				float r_dst = 0.8f;
				float g_dst = 1.0f;
				float b_dst = 0.8f;

				float r_out; 
				float g_out; 
				float b_out; 
				float a_out; 

				// Post-multiply blending
				if (use_post_blend)
				{
					r_out = (r_dst * (1.0f - a_src)) + (r_src * a_src);
					g_out = (g_dst * (1.0f - a_src)) + (g_src * a_src);
					b_out = (b_dst * (1.0f - a_src)) + (b_src * a_src);
					a_out = 1.0f;
				}
				// Pre-multiply blending
				else
				{
					r_out = (r_dst * (1.0f - a_src)) + (r_src * 1.0f);
					g_out = (g_dst * (1.0f - a_src)) + (g_src * 1.0f);
					b_out = (b_dst * (1.0f - a_src)) + (b_src * 1.0f);
					a_out = 1.0f;
				}

				// Clamp color between 0 and 1.0f
				r_out = astc::min(r_out, 1.0f);
				g_out = astc::min(g_out, 1.0f);
				b_out = astc::min(b_out, 1.0f);

				if (use_linear == false)
				{
					r_out = linear_to_srgb(r_out);
					g_out = linear_to_srgb(g_out);
					b_out = linear_to_srgb(b_out);
				}

				pixel_out[0] = (uint8_t)(r_out * 255.0f);
				pixel_out[1] = (uint8_t)(g_out * 255.0f);
				pixel_out[2] = (uint8_t)(b_out * 255.0f);
				pixel_out[3] = (uint8_t)(a_out * 255.0f);
			}
		}
	}
	else
	{
		for (int y = 0; y < dim_y - 1; y++)
		{
			const uint8_t* row_in_0 = data_in + (4 * dim_x * y);
			const uint8_t* row_in_1 = data_in + (4 * dim_x * (y + 1));

			uint8_t* row_out = data_out + (4 * (dim_x - 1) * y);

			for (int x = 0; x < dim_x - 1; x++)
			{
				const uint8_t* pixel_in_00 = row_in_0 + 4 * x;
				const uint8_t* pixel_in_01 = row_in_0 + 4 * (x + 1);
				const uint8_t* pixel_in_10 = row_in_1 + 4 * x;
				const uint8_t* pixel_in_11 = row_in_1 + 4 * (x + 1);

				uint8_t* pixel_out = row_out + 4 * x;

				// Bilinear filter with a half-pixel offset
				float r_src = static_cast<float>(pixel_in_00[0] + pixel_in_01[0] + pixel_in_10[0] + pixel_in_11[0]) / (255.0f * 4.0f);
				float g_src = static_cast<float>(pixel_in_00[1] + pixel_in_01[1] + pixel_in_10[1] + pixel_in_11[1]) / (255.0f * 4.0f);
				float b_src = static_cast<float>(pixel_in_00[2] + pixel_in_01[2] + pixel_in_10[2] + pixel_in_11[2]) / (255.0f * 4.0f);
				float a_src = static_cast<float>(pixel_in_00[3] + pixel_in_01[3] + pixel_in_10[3] + pixel_in_11[3]) / (255.0f * 4.0f);

				if (use_linear == false)
				{
					r_src = srgb_to_linear(r_src);
					g_src = srgb_to_linear(g_src);
					b_src = srgb_to_linear(b_src);
				}

				float r_dst = 0.8f;
				float g_dst = 1.0f;
				float b_dst = 0.8f;

				float r_out; 
				float g_out; 
				float b_out; 
				float a_out; 

				// Post-multiply blending
				if (use_post_blend)
				{
					r_out = (r_dst * (1.0f - a_src)) + (r_src * a_src);
					g_out = (g_dst * (1.0f - a_src)) + (g_src * a_src);
					b_out = (b_dst * (1.0f - a_src)) + (b_src * a_src);
					a_out = 1.0f;
				}
				// Pre-multiply blending
				else
				{
					r_out = (r_dst * (1.0f - a_src)) + (r_src * 1.0f);
					g_out = (g_dst * (1.0f - a_src)) + (g_src * 1.0f);
					b_out = (b_dst * (1.0f - a_src)) + (b_src * 1.0f);
					a_out = 1.0f;
				}

				// Clamp color between 0 and 1.0f
				r_out = astc::min(r_out, 1.0f);
				g_out = astc::min(g_out, 1.0f);
				b_out = astc::min(b_out, 1.0f);

				if (use_linear == false)
				{
					r_out = linear_to_srgb(r_out);
					g_out = linear_to_srgb(g_out);
					b_out = linear_to_srgb(b_out);
				}

				pixel_out[0] = (uint8_t)(r_out * 255.0f);
				pixel_out[1] = (uint8_t)(g_out * 255.0f);
				pixel_out[2] = (uint8_t)(b_out * 255.0f);
				pixel_out[3] = (uint8_t)(a_out * 255.0f);
			}
		}
	}

	// Write out the result
	if (!use_filter)
	{
		stbi_write_png(dst_file, dim_x, dim_y, 4, data_out, 4 * dim_x);
	}
	else
	{
		stbi_write_png(dst_file, dim_x - 1, dim_y - 1, 4, data_out, 4 * (dim_x - 1));
	}


	return 0;
}
