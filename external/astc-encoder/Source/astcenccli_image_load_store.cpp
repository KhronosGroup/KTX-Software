// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2011-2026 Arm Limited
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
 * @brief Functions for loading/storing uncompressed and compressed images.
 */

#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <memory>
#include <new>
#include <sstream>
#include <utility>
#include <vector>

#include "astcenccli_internal.h"

#include "ThirdParty/stb_image.h"
#include "ThirdParty/stb_image_write.h"
#include "ThirdParty/tinyexr.h"

/**
 * @brief Reverse the bytes in a uint32_t value.
 */
static uint32_t reverse_bytes_u32(
	uint32_t val
) {
	return ((val >> 24) & 0x000000FF) |
	       ((val >>  8) & 0x0000FF00) |
	       ((val <<  8) & 0x00FF0000) |
	       ((val << 24) & 0xFF000000);
}

/**
 * @brief Determine the output file name to use for a sliced image write.
 *
 * @param img        The source data for the image.
 * @param filename   The base name of the file to save.
 * @param index      The slice index to write.
 *
 * @return The file name to use when saving the file.
 */
static std::string get_output_filename(
	const astcenc_image* img,
	const char* filename,
	unsigned int index
) {
	if (img->dim_z <= 1)
	{
		return filename;
	}

	std::string fnmod(filename);
	std::string fnext = fnmod.substr(fnmod.find_last_of("."));

	// Remove the extension
	fnmod = fnmod.erase(fnmod.length() - fnext.size());

	// Insert the file index into the base name, then append the extension
	std::stringstream ss;
	ss << fnmod << "_" << std::setw(3) << std::setfill('0') << index << fnext;
	return ss.str();
}

/* ============================================================================
  Image load and store through the stb_image and tinyexr libraries
============================================================================ */

struct tinyexr_image_deleter
{
	void operator()(void* ptr) const
	{
		free(ptr);
	}
};

struct tinyexr_error_deleter
{
	void operator()(const void* ptr) const
	{
		free(const_cast<void*>(ptr));
	}
};

struct stbi_image_deleter
{
	void operator()(void* ptr) const
	{
		stbi_image_free(ptr);
	}
};

/**
 * @brief Load a .exr image using TinyExr to provide the loader.
 *
 * @param      filename          The name of the file to load.
 * @param      y_flip            Should the image be vertically flipped?
 * @param[out] is_hdr            Is this an HDR image load? Always @c true for this function.
 * @param[out] component_count   The number of components in the data.
 *
 * @return The loaded image data in a canonical 4 channel format.
 */
static astcenc_image_ptr load_image_with_tinyexr(
	const char* filename,
	bool y_flip,
	bool& is_hdr,
	unsigned int& component_count
) {
	int dim_x, dim_y;
	float* image_raw;
	const char* err { nullptr };

	int load_res = LoadEXR(&image_raw, &dim_x, &dim_y, filename, &err);
	if (load_res != TINYEXR_SUCCESS)
	{
		std::unique_ptr<const char, tinyexr_error_deleter> err_ptr { err };
		print_error("ERROR: Image load failed '%s' (%s)\n",
		            filename, err_ptr.get() ? err_ptr.get() : "unknown error");
		return nullptr;
	}

	std::unique_ptr<float, tinyexr_image_deleter> image { image_raw };
	auto res_img = astc_img_from_floatx4_array(image.get(), dim_x, dim_y, y_flip);

	is_hdr = true;
	component_count = 4;
	return res_img;
}

/**
 * @brief Load an image using STBImage to provide the loader.
 *
 * @param      filename          The name of the file to load.
 * @param      y_flip            Should the image be vertically flipped?
 * @param[out] is_hdr            Is this an HDR image load?
 * @param[out] component_count   The number of components in the data.
 *
 * @return The loaded image data in a canonical 4 channel format, or @c nullptr on error.
 */
static astcenc_image_ptr load_image_with_stb(
	const char* filename,
	bool y_flip,
	bool& is_hdr,
	unsigned int& component_count
) {
	int dim_x, dim_y;

	if (stbi_is_hdr(filename))
	{
		std::unique_ptr<float, stbi_image_deleter> data {
			stbi_loadf(filename, &dim_x, &dim_y, nullptr, STBI_rgb_alpha)
		};
		if (data)
		{
			auto img = astc_img_from_floatx4_array(data.get(), dim_x, dim_y, y_flip);
			is_hdr = true;
			component_count = 4;
			return img;
		}
	}
	else
	{
		std::unique_ptr<uint8_t, stbi_image_deleter> data {
			stbi_load(filename, &dim_x, &dim_y, nullptr, STBI_rgb_alpha)
		};
		if (data)
		{
			auto img = astc_img_from_unorm8x4_array(data.get(), dim_x, dim_y, y_flip);
			is_hdr = false;
			component_count = 4;
			return img;
		}
	}

	print_error("ERROR: Image load failed '%s' (%s)\n", filename, stbi_failure_reason());
	return nullptr;
}

/**
 * @brief Save an EXR image using TinyExr to provide the store routine.
 *
 * @param img        The source data for the image.
 * @param filename   The name of the file to save.
 * @param y_flip     Should the image be vertically flipped?
 *
 * @return @c true if the image saved OK, @c false on error.
 */
static bool store_exr_image_with_tinyexr(
	const astcenc_image* img,
	const char* filename,
	int y_flip
) {
	int res { 0 };

	for (unsigned int i = 0; i < img->dim_z; i++)
	{
		std::string fnmod = get_output_filename(img, filename, i);
		std::vector<float> buf = floatx4_array_from_astc_img(img, y_flip, i);

		res = SaveEXR(buf.data(), img->dim_x, img->dim_y, 4, 1, fnmod.c_str(), nullptr);
		if (res < 0)
		{
			break;
		}
	}

	return res >= 0;
}

/**
 * @brief Save a PNG image using STBImageWrite to provide the store routine.
 *
 * @param img        The source data for the image.
 * @param filename   The name of the file to save.
 * @param y_flip     Should the image be vertically flipped?
 *
 * @return @c true if the image saved OK, @c false on error.
 */
static bool store_png_image_with_stb(
	const astcenc_image* img,
	const char* filename,
	int y_flip
) {
	int res { 0 };

	assert(img->data_type == ASTCENC_TYPE_U8);

	for (unsigned int i = 0; i < img->dim_z; i++)
	{
		std::string fnmod = get_output_filename(img, filename, i);
		uint8_t* buf = reinterpret_cast<uint8_t*>(img->data[i]);

		stbi_flip_vertically_on_write(y_flip);
		res = stbi_write_png(fnmod.c_str(), img->dim_x, img->dim_y, 4, buf, img->dim_x * 4);
		if (res == 0)
		{
			break;
		}
	}

	return res != 0;
}

/**
 * @brief Save a TGA image using STBImageWrite to provide the store routine.
 *
 * @param img        The source data for the image.
 * @param filename   The name of the file to save.
 * @param y_flip     Should the image be vertically flipped?
 *
 * @return @c true if the image saved OK, @c false on error.
 */
static bool store_tga_image_with_stb(
	const astcenc_image* img,
	const char* filename,
	int y_flip
) {
	int res { 0 };

	assert(img->data_type == ASTCENC_TYPE_U8);

	for (unsigned int i = 0; i < img->dim_z; i++)
	{
		std::string fnmod = get_output_filename(img, filename, i);
		uint8_t* buf = reinterpret_cast<uint8_t*>(img->data[i]);

		stbi_flip_vertically_on_write(y_flip);
		res = stbi_write_tga(fnmod.c_str(), img->dim_x, img->dim_y, 4, buf);
		if (res == 0)
		{
			break;
		}
	}

	return res != 0;
}

/**
 * @brief Save a BMP image using STBImageWrite to provide the store routine.
 *
 * @param img        The source data for the image.
 * @param filename   The name of the file to save.
 * @param y_flip     Should the image be vertically flipped?
 *
 * @return @c true if the image saved OK, @c false on error.
 */
static bool store_bmp_image_with_stb(
	const astcenc_image* img,
	const char* filename,
	int y_flip
) {
	int res { 0 };

	assert(img->data_type == ASTCENC_TYPE_U8);

	for (unsigned int i = 0; i < img->dim_z; i++)
	{
		std::string fnmod = get_output_filename(img, filename, i);
		uint8_t* buf = reinterpret_cast<uint8_t*>(img->data[i]);

		stbi_flip_vertically_on_write(y_flip);
		res = stbi_write_bmp(fnmod.c_str(), img->dim_x, img->dim_y, 4, buf);
		if (res == 0)
		{
			break;
		}
	}

	return res != 0;
}

/**
 * @brief Save a HDR image using STBImageWrite to provide the store routine.
 *
 * @param img        The source data for the image.
 * @param filename   The name of the file to save.
 * @param y_flip     Should the image be vertically flipped?
 *
 * @return @c true if the image saved OK, @c false on error.
 */
static bool store_hdr_image_with_stb(
	const astcenc_image* img,
	const char* filename,
	int y_flip
) {
	int res { 0 };

	for (unsigned int i = 0; i < img->dim_z; i++)
	{
		std::string fnmod = get_output_filename(img, filename, i);
		std::vector<float> buf = floatx4_array_from_astc_img(img, y_flip, i);

		res = stbi_write_hdr(fnmod.c_str(), img->dim_x, img->dim_y, 4, buf.data());
		if (res == 0)
		{
			break;
		}
	}

	return res != 0;
}

/* ============================================================================
Native Load and store of KTX and DDS file formats.

Unlike "regular" 2D image formats, which are mostly supported through stb_image
and tinyexr, these formats are supported directly; this involves a relatively
large number of pixel formats.

The following restrictions apply to loading of these file formats:

    * Only uncompressed data supported
    * Only first mipmap in mipmap pyramid supported
    * KTX: Cube-map arrays are not supported
============================================================================ */
enum scanline_transfer
{
	R8_TO_RGBA8,
	RG8_TO_RGBA8,
	RGB8_TO_RGBA8,
	RGBA8_TO_RGBA8,
	BGR8_TO_RGBA8,
	BGRA8_TO_RGBA8,
	L8_TO_RGBA8,
	LA8_TO_RGBA8,

	RGBX8_TO_RGBA8,
	BGRX8_TO_RGBA8,

	R16_TO_RGBA16F,
	RG16_TO_RGBA16F,
	RGB16_TO_RGBA16F,
	RGBA16_TO_RGBA16F,
	BGR16_TO_RGBA16F,
	BGRA16_TO_RGBA16F,
	L16_TO_RGBA16F,
	LA16_TO_RGBA16F,

	R16F_TO_RGBA16F,
	RG16F_TO_RGBA16F,
	RGB16F_TO_RGBA16F,
	RGBA16F_TO_RGBA16F,
	BGR16F_TO_RGBA16F,
	BGRA16F_TO_RGBA16F,
	L16F_TO_RGBA16F,
	LA16F_TO_RGBA16F,

	R32F_TO_RGBA16F,
	RG32F_TO_RGBA16F,
	RGB32F_TO_RGBA16F,
	RGBA32F_TO_RGBA16F,
	BGR32F_TO_RGBA16F,
	BGRA32F_TO_RGBA16F,
	L32F_TO_RGBA16F,
	LA32F_TO_RGBA16F
};

/**
 * @brief Copy a scanline from a source file and expand to a canonical format.
 *
 * Outputs are always 4 component RGBA, stored as U8 (LDR) or FP16 (HDR).
 *
 * @param[out] dst           The start of the line to store to.
 * @param      src           The start of the line to load.
 * @param      pixel_count   The number of pixels in the scanline.
 * @param      method        The conversion function.
 */
static void copy_scanline(
	void* dst,
	const void* src,
	size_t pixel_count,
	scanline_transfer method
) {

#define id(x) (x)
#define u16_sf16(x) float_to_float16(x * (1.0f/65535.0f))
#define f32_sf16(x) float_to_float16(x)

#define COPY_R(dsttype, srctype, convfunc, oneval) \
	do { \
		const srctype* s = reinterpret_cast<const srctype*>(src); \
		dsttype* d = reinterpret_cast<dsttype*>(dst); \
		for (size_t i = 0; i < pixel_count; i++) \
		{ \
			d[4 * i    ] = convfunc(s[i]); \
			d[4 * i + 1] = 0;              \
			d[4 * i + 2] = 0;              \
			d[4 * i + 3] = oneval;         \
		} \
	} while (0); \
	break

#define COPY_RG(dsttype, srctype, convfunc, oneval) \
	do { \
		const srctype* s = reinterpret_cast<const srctype*>(src); \
		dsttype* d = reinterpret_cast<dsttype*>(dst); \
		for (size_t i = 0; i < pixel_count; i++) \
		{ \
			d[4 * i    ] = convfunc(s[2 * i    ]); \
			d[4 * i + 1] = convfunc(s[2 * i + 1]); \
			d[4 * i + 2] = 0;                      \
			d[4 * i + 3] = oneval;                 \
		} \
	} while (0); \
	break

#define COPY_RGB(dsttype, srctype, convfunc, oneval) \
	do { \
		const srctype* s = reinterpret_cast<const srctype*>(src); \
		dsttype* d = reinterpret_cast<dsttype*>(dst); \
		for (size_t i = 0; i < pixel_count; i++) \
		{ \
			d[4 * i    ] = convfunc(s[3 * i    ]); \
			d[4 * i + 1] = convfunc(s[3 * i + 1]); \
			d[4 * i + 2] = convfunc(s[3 * i + 2]); \
			d[4 * i + 3] = oneval;                 \
		} \
	} while (0); \
	break

#define COPY_BGR(dsttype, srctype, convfunc, oneval) \
	do { \
		const srctype* s = reinterpret_cast<const srctype*>(src); \
		dsttype* d = reinterpret_cast<dsttype*>(dst); \
		for (size_t i = 0; i < pixel_count; i++)\
		{ \
			d[4 * i    ] = convfunc(s[3 * i + 2]); \
			d[4 * i + 1] = convfunc(s[3 * i + 1]); \
			d[4 * i + 2] = convfunc(s[3 * i    ]); \
			d[4 * i + 3] = oneval;                 \
		} \
	} while (0); \
	break

#define COPY_RGBX(dsttype, srctype, convfunc, oneval) \
	do { \
		const srctype* s = reinterpret_cast<const srctype*>(src); \
		dsttype* d = reinterpret_cast<dsttype*>(dst); \
		for (size_t i = 0; i < pixel_count; i++)\
		{ \
			d[4 * i    ] = convfunc(s[4 * i    ]); \
			d[4 * i + 1] = convfunc(s[4 * i + 1]); \
			d[4 * i + 2] = convfunc(s[4 * i + 2]); \
			d[4 * i + 3] = oneval;                 \
		} \
	} while (0); \
	break

#define COPY_BGRX(dsttype, srctype, convfunc, oneval) \
	do { \
		const srctype* s = reinterpret_cast<const srctype*>(src); \
		dsttype* d = reinterpret_cast<dsttype*>(dst); \
		for (size_t i = 0; i < pixel_count; i++)\
		{ \
			d[4 * i    ] = convfunc(s[4 * i + 2]); \
			d[4 * i + 1] = convfunc(s[4 * i + 1]); \
			d[4 * i + 2] = convfunc(s[4 * i    ]); \
			d[4 * i + 3] = oneval;                 \
		} \
	} while (0); \
	break

#define COPY_RGBA(dsttype, srctype, convfunc, oneval) \
	do { \
		const srctype* s = reinterpret_cast<const srctype*>(src); \
		dsttype* d = reinterpret_cast<dsttype*>(dst); \
		for (size_t i = 0; i < pixel_count; i++) \
		{ \
			d[4 * i    ] = convfunc(s[4 * i    ]); \
			d[4 * i + 1] = convfunc(s[4 * i + 1]); \
			d[4 * i + 2] = convfunc(s[4 * i + 2]); \
			d[4 * i + 3] = convfunc(s[4 * i + 3]); \
		} \
	} while (0); \
	break

#define COPY_BGRA(dsttype, srctype, convfunc, oneval) \
	do { \
		const srctype* s = reinterpret_cast<const srctype*>(src); \
		dsttype* d = reinterpret_cast<dsttype*>(dst); \
		for (size_t i = 0; i < pixel_count; i++) \
		{ \
			d[4 * i    ] = convfunc(s[4 * i + 2]); \
			d[4 * i + 1] = convfunc(s[4 * i + 1]); \
			d[4 * i + 2] = convfunc(s[4 * i    ]); \
			d[4 * i + 3] = convfunc(s[4 * i + 3]); \
		} \
	} while (0); \
	break

#define COPY_L(dsttype, srctype, convfunc, oneval) \
	do { \
		const srctype* s = reinterpret_cast<const srctype*>(src); \
		dsttype* d = reinterpret_cast<dsttype*>(dst); \
		for (size_t i = 0; i < pixel_count; i++) \
		{ \
			d[4 * i    ] = convfunc(s[i]); \
			d[4 * i + 1] = convfunc(s[i]); \
			d[4 * i + 2] = convfunc(s[i]); \
			d[4 * i + 3] = oneval;         \
		} \
	} while (0); \
	break

#define COPY_LA(dsttype, srctype, convfunc, oneval) \
	do { \
		const srctype* s = reinterpret_cast<const srctype*>(src); \
		dsttype* d = reinterpret_cast<dsttype*>(dst); \
		for (size_t i = 0; i < pixel_count; i++) \
		{ \
			d[4 * i    ] = convfunc(s[2 * i    ]); \
			d[4 * i + 1] = convfunc(s[2 * i    ]); \
			d[4 * i + 2] = convfunc(s[2 * i    ]); \
			d[4 * i + 3] = convfunc(s[2 * i + 1]); \
		} \
	} while (0); \
	break

	switch (method)
	{
	case R8_TO_RGBA8:
		COPY_R(uint8_t, uint8_t, id, 0xFF);
	case RG8_TO_RGBA8:
		COPY_RG(uint8_t, uint8_t, id, 0xFF);
	case RGB8_TO_RGBA8:
		COPY_RGB(uint8_t, uint8_t, id, 0xFF);
	case RGBA8_TO_RGBA8:
		COPY_RGBA(uint8_t, uint8_t, id, 0xFF);
	case BGR8_TO_RGBA8:
		COPY_BGR(uint8_t, uint8_t, id, 0xFF);
	case BGRA8_TO_RGBA8:
		COPY_BGRA(uint8_t, uint8_t, id, 0xFF);
	case RGBX8_TO_RGBA8:
		COPY_RGBX(uint8_t, uint8_t, id, 0xFF);
	case BGRX8_TO_RGBA8:
		COPY_BGRX(uint8_t, uint8_t, id, 0xFF);
	case L8_TO_RGBA8:
		COPY_L(uint8_t, uint8_t, id, 0xFF);
	case LA8_TO_RGBA8:
		COPY_LA(uint8_t, uint8_t, id, 0xFF);

	case R16F_TO_RGBA16F:
		COPY_R(uint16_t, uint16_t, id, 0x3C00);
	case RG16F_TO_RGBA16F:
		COPY_RG(uint16_t, uint16_t, id, 0x3C00);
	case RGB16F_TO_RGBA16F:
		COPY_RGB(uint16_t, uint16_t, id, 0x3C00);
	case RGBA16F_TO_RGBA16F:
		COPY_RGBA(uint16_t, uint16_t, id, 0x3C00);
	case BGR16F_TO_RGBA16F:
		COPY_BGR(uint16_t, uint16_t, id, 0x3C00);
	case BGRA16F_TO_RGBA16F:
		COPY_BGRA(uint16_t, uint16_t, id, 0x3C00);
	case L16F_TO_RGBA16F:
		COPY_L(uint16_t, uint16_t, id, 0x3C00);
	case LA16F_TO_RGBA16F:
		COPY_LA(uint16_t, uint16_t, id, 0x3C00);

	case R16_TO_RGBA16F:
		COPY_R(uint16_t, uint16_t, u16_sf16, 0x3C00);
	case RG16_TO_RGBA16F:
		COPY_RG(uint16_t, uint16_t, u16_sf16, 0x3C00);
	case RGB16_TO_RGBA16F:
		COPY_RGB(uint16_t, uint16_t, u16_sf16, 0x3C00);
	case RGBA16_TO_RGBA16F:
		COPY_RGBA(uint16_t, uint16_t, u16_sf16, 0x3C00);
	case BGR16_TO_RGBA16F:
		COPY_BGR(uint16_t, uint16_t, u16_sf16, 0x3C00);
	case BGRA16_TO_RGBA16F:
		COPY_BGRA(uint16_t, uint16_t, u16_sf16, 0x3C00);
	case L16_TO_RGBA16F:
		COPY_L(uint16_t, uint16_t, u16_sf16, 0x3C00);
	case LA16_TO_RGBA16F:
		COPY_LA(uint16_t, uint16_t, u16_sf16, 0x3C00);

	case R32F_TO_RGBA16F:
		COPY_R(uint16_t, float, f32_sf16, 0x3C00);
	case RG32F_TO_RGBA16F:
		COPY_RG(uint16_t, float, f32_sf16, 0x3C00);
	case RGB32F_TO_RGBA16F:
		COPY_RGB(uint16_t, float, f32_sf16, 0x3C00);
	case RGBA32F_TO_RGBA16F:
		COPY_RGBA(uint16_t, float, f32_sf16, 0x3C00);
	case BGR32F_TO_RGBA16F:
		COPY_BGR(uint16_t, float, f32_sf16, 0x3C00);
	case BGRA32F_TO_RGBA16F:
		COPY_BGRA(uint16_t, float, f32_sf16, 0x3C00);
	case L32F_TO_RGBA16F:
		COPY_L(uint16_t, float, f32_sf16, 0x3C00);
	case LA32F_TO_RGBA16F:
		COPY_LA(uint16_t, float, f32_sf16, 0x3C00);
	}
}

/**
 * @brief Swap endianness of N two byte values.
 *
 * @param[in,out] dataptr      The data to convert.
 * @param         byte_count   The number of bytes to convert.
 */
static void switch_endianness2(
	void* dataptr,
	size_t byte_count
) {
	uint8_t* data = reinterpret_cast<uint8_t*>(dataptr);
	for (size_t i = 0; i < byte_count / 2; i++)
	{
		uint8_t d0 = data[0];
		uint8_t d1 = data[1];
		data[0] = d1;
		data[1] = d0;
		data += 2;
	}
}

/**
 * @brief Swap endianness of N four byte values.
 *
 * @param[in,out] dataptr      The data to convert.
 * @param         byte_count   The number of bytes to convert.
 */
static void switch_endianness4(
	void* dataptr,
	size_t byte_count
) {
	uint8_t* data = reinterpret_cast<uint8_t*>(dataptr);
	for (size_t i = 0; i < byte_count / 4; i++)
	{
		uint8_t d0 = data[0];
		uint8_t d1 = data[1];
		uint8_t d2 = data[2];
		uint8_t d3 = data[3];
		data[0] = d3;
		data[1] = d2;
		data[2] = d1;
		data[3] = d0;
		data += 4;
	}
}

// Khronos enums
#define GL_RED                                      0x1903
#define GL_RG                                       0x8227
#define GL_RGB                                      0x1907
#define GL_RGBA                                     0x1908
#define GL_BGR                                      0x80E0
#define GL_BGRA                                     0x80E1
#define GL_LUMINANCE                                0x1909
#define GL_LUMINANCE_ALPHA                          0x190A

#define GL_R8                                       0x8229
#define GL_RG8                                      0x822B
#define GL_RGB8                                     0x8051
#define GL_RGBA8                                    0x8058

#define GL_R16F                                     0x822D
#define GL_RG16F                                    0x822F
#define GL_RGB16F                                   0x881B
#define GL_RGBA16F                                  0x881A

#define GL_UNSIGNED_BYTE                            0x1401
#define GL_UNSIGNED_SHORT                           0x1403
#define GL_HALF_FLOAT                               0x140B
#define GL_FLOAT                                    0x1406

#define GL_COMPRESSED_RGBA_ASTC_4x4                 0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4                 0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5                 0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5                 0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6                 0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5                 0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6                 0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8                 0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5                0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6                0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8                0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10               0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10               0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12               0x93BD

#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4         0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4         0x93D1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5         0x93D2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5         0x93D3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6         0x93D4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5         0x93D5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6         0x93D6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8         0x93D7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5        0x93D8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6        0x93D9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8        0x93DA
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10       0x93DB
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10       0x93DC
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12       0x93DD

#define GL_COMPRESSED_RGBA_ASTC_3x3x3_OES           0x93C0
#define GL_COMPRESSED_RGBA_ASTC_4x3x3_OES           0x93C1
#define GL_COMPRESSED_RGBA_ASTC_4x4x3_OES           0x93C2
#define GL_COMPRESSED_RGBA_ASTC_4x4x4_OES           0x93C3
#define GL_COMPRESSED_RGBA_ASTC_5x4x4_OES           0x93C4
#define GL_COMPRESSED_RGBA_ASTC_5x5x4_OES           0x93C5
#define GL_COMPRESSED_RGBA_ASTC_5x5x5_OES           0x93C6
#define GL_COMPRESSED_RGBA_ASTC_6x5x5_OES           0x93C7
#define GL_COMPRESSED_RGBA_ASTC_6x6x5_OES           0x93C8
#define GL_COMPRESSED_RGBA_ASTC_6x6x6_OES           0x93C9

#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES   0x93E0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES   0x93E1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES   0x93E2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES   0x93E3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES   0x93E4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES   0x93E5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES   0x93E6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES   0x93E7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES   0x93E8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES   0x93E9

struct format_entry
{
	unsigned int x;
	unsigned int y;
	unsigned int z;
	bool is_srgb;
	unsigned int format;
};

static const std::array<format_entry, 48> ASTC_FORMATS =
{{
	// 2D Linear RGB
	{ 4,  4,  1, false, GL_COMPRESSED_RGBA_ASTC_4x4},
	{ 5,  4,  1, false, GL_COMPRESSED_RGBA_ASTC_5x4},
	{ 5,  5,  1, false, GL_COMPRESSED_RGBA_ASTC_5x5},
	{ 6,  5,  1, false, GL_COMPRESSED_RGBA_ASTC_6x5},
	{ 6,  6,  1, false, GL_COMPRESSED_RGBA_ASTC_6x6},
	{ 8,  5,  1, false, GL_COMPRESSED_RGBA_ASTC_8x5},
	{ 8,  6,  1, false, GL_COMPRESSED_RGBA_ASTC_8x6},
	{ 8,  8,  1, false, GL_COMPRESSED_RGBA_ASTC_8x8},
	{10,  5,  1, false, GL_COMPRESSED_RGBA_ASTC_10x5},
	{10,  6,  1, false, GL_COMPRESSED_RGBA_ASTC_10x6},
	{10,  8,  1, false, GL_COMPRESSED_RGBA_ASTC_10x8},
	{10, 10,  1, false, GL_COMPRESSED_RGBA_ASTC_10x10},
	{12, 10,  1, false, GL_COMPRESSED_RGBA_ASTC_12x10},
	{12, 12,  1, false, GL_COMPRESSED_RGBA_ASTC_12x12},
	// 2D SRGB
	{ 4,  4,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4},
	{ 5,  4,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4},
	{ 5,  5,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5},
	{ 6,  5,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5},
	{ 6,  6,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6},
	{ 8,  5,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5},
	{ 8,  6,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6},
	{ 8,  8,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8},
	{10,  5,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5},
	{10,  6,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6},
	{10,  8,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8},
	{10, 10,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10},
	{12, 10,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10},
	{12, 12,  1,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12},
	// 3D Linear RGB
	{ 3,  3,  3, false, GL_COMPRESSED_RGBA_ASTC_3x3x3_OES},
	{ 4,  3,  3, false, GL_COMPRESSED_RGBA_ASTC_4x3x3_OES},
	{ 4,  4,  3, false, GL_COMPRESSED_RGBA_ASTC_4x4x3_OES},
	{ 4,  4,  4, false, GL_COMPRESSED_RGBA_ASTC_4x4x4_OES},
	{ 5,  4,  4, false, GL_COMPRESSED_RGBA_ASTC_5x4x4_OES},
	{ 5,  5,  4, false, GL_COMPRESSED_RGBA_ASTC_5x5x4_OES},
	{ 5,  5,  5, false, GL_COMPRESSED_RGBA_ASTC_5x5x5_OES},
	{ 6,  5,  5, false, GL_COMPRESSED_RGBA_ASTC_6x5x5_OES},
	{ 6,  6,  5, false, GL_COMPRESSED_RGBA_ASTC_6x6x5_OES},
	{ 6,  6,  6, false, GL_COMPRESSED_RGBA_ASTC_6x6x6_OES},
	// 3D SRGB
	{ 3,  3,  3,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES},
	{ 4,  3,  3,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES},
	{ 4,  4,  3,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES},
	{ 4,  4,  4,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES},
	{ 5,  4,  4,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES},
	{ 5,  5,  4,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES},
	{ 5,  5,  5,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES},
	{ 6,  5,  5,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES},
	{ 6,  6,  5,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES},
	{ 6,  6,  6,  true, GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES}
}};

static const format_entry* get_format(
	unsigned int format
) {
	for (auto& it : ASTC_FORMATS)
	{
		if (it.format == format)
		{
			return &it;
		}
	}
	return nullptr;
}

static unsigned int get_format(
	unsigned int x,
	unsigned int y,
	unsigned int z,
	bool is_srgb
) {
	for (auto& it : ASTC_FORMATS)
	{
		if ((it.x == x) && (it.y == y) && (it.z == z) && (it.is_srgb == is_srgb))
		{
			return it.format;
		}
	}
	return 0;
}

struct ktx_header
{
	uint8_t magic[12];
	uint32_t endianness;				// should be 0x04030201; if it is instead 0x01020304, then the endianness of everything must be switched.
	uint32_t gl_type;					// 0 for compressed textures, otherwise value from table 3.2 (page 162) of OpenGL 4.0 spec
	uint32_t gl_type_size;				// size of data elements to do endianness swap on (1=endian-neutral data)
	uint32_t gl_format;					// 0 for compressed textures, otherwise value from table 3.3 (page 163) of OpenGL spec
	uint32_t gl_internal_format;		// sized-internal-format, corresponding to table 3.12 to 3.14 (pages 182-185) of OpenGL spec
	uint32_t gl_base_internal_format;	// unsized-internal-format: corresponding to table 3.11 (page 179) of OpenGL spec
	uint32_t pixel_width;				// texture dimensions; not rounded up to block size for compressed.
	uint32_t pixel_height;				// must be 0 for 1D textures.
	uint32_t pixel_depth;				// must be 0 for 1D, 2D and cubemap textures.
	uint32_t number_of_array_elements;	// 0 if not a texture array
	uint32_t number_of_faces;			// 6 for cubemaps, 1 for non-cubemaps
	uint32_t number_of_mipmap_levels;	// 0 or 1 for non-mipmapped textures; 0 indicates that auto-mipmap-gen should be done at load time.
	uint32_t bytes_of_key_value_data;	// size in bytes of the key-and-value area immediately following the header.
};

// Magic 12-byte sequence that must appear at the beginning of every KTX file.
static uint8_t ktx_magic[12] {
	0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};

static void ktx_header_switch_endianness(ktx_header * kt)
{
	#define REV(x) kt->x = reverse_bytes_u32(kt->x)
	REV(endianness);
	REV(gl_type);
	REV(gl_type_size);
	REV(gl_format);
	REV(gl_internal_format);
	REV(gl_base_internal_format);
	REV(pixel_width);
	REV(pixel_height);
	REV(pixel_depth);
	REV(number_of_array_elements);
	REV(number_of_faces);
	REV(number_of_mipmap_levels);
	REV(bytes_of_key_value_data);
	#undef REV
}

/**
 * @brief Load an uncompressed KTX image using the local custom loader.
 *
 * @param      filename          The name of the file to load.
 * @param      y_flip            Should the image be vertically flipped?
 * @param[out] is_hdr            Is this an HDR image load?
 * @param[out] component_count   The number of components in the data.
 *
 * @return The loaded image data in a canonical 4 channel format, or @c nullptr on error.
 */
static astcenc_image_ptr load_ktx_uncompressed_image(
	const char* filename,
	bool y_flip,
	bool& is_hdr,
	unsigned int& component_count
) {
	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if (!file)
	{
		print_error("ERROR: File open failed '%s'\n", filename);
		return nullptr;
	}

	ktx_header hdr;
	file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return nullptr;
	}

	if (memcmp(hdr.magic, ktx_magic, 12) != 0 || (hdr.endianness != 0x04030201 && hdr.endianness != 0x01020304))
	{
		print_error("ERROR: Image header corrupt '%s'\n", filename);
		return nullptr;
	}

	bool switch_endianness = false;
	if (hdr.endianness == 0x01020304)
	{
		ktx_header_switch_endianness(&hdr);
		switch_endianness = true;
	}

	if (hdr.gl_type == 0 || hdr.gl_format == 0)
	{
		print_error("ERROR: Image uses unsupported KTX format '%s'\n", filename);
		return nullptr;
	}

	// Supported formats are all pairings of:
	//   type=(UNSIGNED_BYTE, UNSIGNED_SHORT, HALF_FLOAT, FLOAT)
	//   gl_format=(RED, RG, RGB, RGBA, BGR, BGRA)

	unsigned int components;
	switch (hdr.gl_format)
	{
	case GL_RED:
		components = 1;
		break;
	case GL_RG:
		components = 2;
		break;
	case GL_RGB:
		components = 3;
		break;
	case GL_RGBA:
		components = 4;
		break;
	case GL_BGR:
		components = 3;
		break;
	case GL_BGRA:
		components = 4;
		break;
	case GL_LUMINANCE:
		components = 1;
		break;
	case GL_LUMINANCE_ALPHA:
		components = 2;
		break;
	default:
		print_error("ERROR: Image uses unsupported KTX format '%s'\n", filename);
		return nullptr;
	}

	// Although these are set up later, use default initializer to remove warnings
	unsigned int bitness = 8;              // Internal precision after conversion
	unsigned int bytes_per_component = 1;  // Bytes per component in the KTX file
	scanline_transfer copy_method = R8_TO_RGBA8;

	switch (hdr.gl_type)
	{
	case GL_UNSIGNED_BYTE:
		{
			bitness = 8;
			bytes_per_component = 1;
			switch (hdr.gl_format)
			{
			case GL_RED:
				copy_method = R8_TO_RGBA8;
				break;
			case GL_RG:
				copy_method = RG8_TO_RGBA8;
				break;
			case GL_RGB:
				copy_method = RGB8_TO_RGBA8;
				break;
			case GL_RGBA:
				copy_method = RGBA8_TO_RGBA8;
				break;
			case GL_BGR:
				copy_method = BGR8_TO_RGBA8;
				break;
			case GL_BGRA:
				copy_method = BGRA8_TO_RGBA8;
				break;
			case GL_LUMINANCE:
				copy_method = L8_TO_RGBA8;
				break;
			case GL_LUMINANCE_ALPHA:
				copy_method = LA8_TO_RGBA8;
				break;
			}
			break;
		}
	case GL_UNSIGNED_SHORT:
		{
			bitness = 16;
			bytes_per_component = 2;
			switch (hdr.gl_format)
			{
			case GL_RED:
				copy_method = R16_TO_RGBA16F;
				break;
			case GL_RG:
				copy_method = RG16_TO_RGBA16F;
				break;
			case GL_RGB:
				copy_method = RGB16_TO_RGBA16F;
				break;
			case GL_RGBA:
				copy_method = RGBA16_TO_RGBA16F;
				break;
			case GL_BGR:
				copy_method = BGR16_TO_RGBA16F;
				break;
			case GL_BGRA:
				copy_method = BGRA16_TO_RGBA16F;
				break;
			case GL_LUMINANCE:
				copy_method = L16_TO_RGBA16F;
				break;
			case GL_LUMINANCE_ALPHA:
				copy_method = LA16_TO_RGBA16F;
				break;
			}
			break;
		}
	case GL_HALF_FLOAT:
		{
			bitness = 16;
			bytes_per_component = 2;
			switch (hdr.gl_format)
			{
			case GL_RED:
				copy_method = R16F_TO_RGBA16F;
				break;
			case GL_RG:
				copy_method = RG16F_TO_RGBA16F;
				break;
			case GL_RGB:
				copy_method = RGB16F_TO_RGBA16F;
				break;
			case GL_RGBA:
				copy_method = RGBA16F_TO_RGBA16F;
				break;
			case GL_BGR:
				copy_method = BGR16F_TO_RGBA16F;
				break;
			case GL_BGRA:
				copy_method = BGRA16F_TO_RGBA16F;
				break;
			case GL_LUMINANCE:
				copy_method = L16F_TO_RGBA16F;
				break;
			case GL_LUMINANCE_ALPHA:
				copy_method = LA16F_TO_RGBA16F;
				break;
			}
			break;
		}
	case GL_FLOAT:
		{
			bitness = 16;
			bytes_per_component = 4;
			switch (hdr.gl_format)
			{
			case GL_RED:
				copy_method = R32F_TO_RGBA16F;
				break;
			case GL_RG:
				copy_method = RG32F_TO_RGBA16F;
				break;
			case GL_RGB:
				copy_method = RGB32F_TO_RGBA16F;
				break;
			case GL_RGBA:
				copy_method = RGBA32F_TO_RGBA16F;
				break;
			case GL_BGR:
				copy_method = BGR32F_TO_RGBA16F;
				break;
			case GL_BGRA:
				copy_method = BGRA32F_TO_RGBA16F;
				break;
			case GL_LUMINANCE:
				copy_method = L32F_TO_RGBA16F;
				break;
			case GL_LUMINANCE_ALPHA:
				copy_method = LA32F_TO_RGBA16F;
				break;
			}
			break;
		}
	default:
		print_error("ERROR: Image uses unsupported KTX format '%s'\n", filename);
		return nullptr;
	}

	if (hdr.number_of_mipmap_levels > 1)
	{
		printf("WARNING: Only the first of %u mipmap levels will be compressed '%s'.\n", hdr.number_of_mipmap_levels, filename);
	}

	if (hdr.number_of_array_elements > 1)
	{
		printf("WARNING: Only the first of %u array layers will be compressed '%s'.\n", hdr.number_of_array_elements, filename);
	}

	if (hdr.number_of_faces > 1)
	{
		printf("WARNING: Only the first of %u cube faces will be compressed '%s'.\n", hdr.number_of_faces, filename);
	}

	unsigned int dim_x = hdr.pixel_width;
	unsigned int dim_y = astc::max(hdr.pixel_height, 1u);
	unsigned int dim_z = astc::max(hdr.pixel_depth, 1u);

	// ignore the key/value data
	file.seekg(hdr.bytes_of_key_value_data, std::ios::cur);
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return nullptr;
	}

	uint32_t specified_bytes_per_image = 0;
	file.read(reinterpret_cast<char*>(&specified_bytes_per_image), sizeof(specified_bytes_per_image));
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return nullptr;
	}

	if (switch_endianness)
	{
		specified_bytes_per_image = reverse_bytes_u32(specified_bytes_per_image);
	}

	// Compute surface size, checking for overflow caused by bad user-defined sizes
	// These values are trusted and cannot overflow
	size_t bytes_per_pixel = bytes_per_component * components;

	bool overflow { false };

	size_t bytes_per_row = astc::mul_safe(bytes_per_pixel, dim_x, overflow);
	size_t bytes_per_plane = astc::mul_safe(bytes_per_row, dim_y, overflow);
	size_t bytes_per_image = astc::mul_safe(bytes_per_plane, dim_z, overflow);

	// Also verify that our output plane allocation does not overflow because
	// this always uses 4 components which can be more than the input file used
	size_t plane_texels = astc::mul_safe(dim_x, dim_y, overflow);
	astc::mul_safe(plane_texels, 4, overflow);

	if (overflow || bytes_per_image != specified_bytes_per_image)
	{
		print_error("ERROR: Image header corrupt '%s'\n", filename);
		return nullptr;
	}

	std::unique_ptr<uint8_t[]> buf;
	try
	{
		buf = std::make_unique<uint8_t[]>(bytes_per_image);
	}
	catch (const std::bad_alloc &e)
	{
		ASTCENC_UNUSED(e);
		print_error("ERROR: Image memory allocation failed '%s'\n", filename);
		return nullptr;
	}

	file.read(reinterpret_cast<char*>(buf.get()), bytes_per_image);
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return nullptr;
	}

	// Perform an endianness swap on the surface if needed.
	if (switch_endianness)
	{
		if (hdr.gl_type_size == 2)
		{
			switch_endianness2(buf.get(), bytes_per_image);
		}

		if (hdr.gl_type_size == 4)
		{
			switch_endianness4(buf.get(), bytes_per_image);
		}
	}

	// Transfer data from the surface to our own image data structure
	astcenc_image_ptr astc_img;
	try
	{
		astc_img = alloc_image(bitness, dim_x, dim_y, dim_z);
	}
	catch (const std::bad_alloc &e)
	{
		ASTCENC_UNUSED(e);
		print_error("ERROR: Image memory allocation failed '%s'\n", filename);
		return nullptr;
	}

	// TODO: Change astenc_image struct to store size_t rather than unsigned
	// int then make dim_x/y/z size_t at the start of the function. This is an
	// API break, so needs to wait until we make a major version.
	size_t dim_y_sz = dim_y;
	size_t dim_z_sz = dim_z;

	for (size_t z = 0; z < dim_z_sz; z++)
	{
		for (size_t y = 0; y < dim_y_sz; y++)
		{
			size_t mod_y = y_flip ? dim_y_sz - y - 1 : y;
			void *dst;

			if (astc_img->data_type == ASTCENC_TYPE_U8)
			{
				uint8_t* data8 = static_cast<uint8_t*>(astc_img->data[z]);
				dst = static_cast<void*>(&data8[4 * dim_x * mod_y]);
			}
			else // if (astc_img->data_type == ASTCENC_TYPE_F16)
			{
				assert(astc_img->data_type == ASTCENC_TYPE_F16);
				uint16_t* data16 = static_cast<uint16_t*>(astc_img->data[z]);
				dst = static_cast<void*>(&data16[4 * dim_x * mod_y]);
			}

			uint8_t *src = buf.get() + (z * bytes_per_plane) + (y * bytes_per_row);
			copy_scanline(dst, src, dim_x, copy_method);
		}
	}

	is_hdr = bitness >= 16;
	component_count = components;
	return astc_img;
}

/**
 * @brief Load a KTX compressed image using the local custom loader.
 *
 * @param      filename          The name of the file to load.
 * @param[out] is_srgb           @c true if this is an sRGB image, @c false otherwise.
 * @param[out] img               The output image to populate.
 *
 * @return @c true on error, @c false otherwise.
 */
bool load_ktx_compressed_image(
	const char* filename,
	bool& is_srgb,
	astc_compressed_image& img
) {
	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if (!file)
	{
		print_error("ERROR: File open failed '%s'\n", filename);
		return true;
	}

	ktx_header hdr;
	file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return true;
	}

	if (memcmp(hdr.magic, ktx_magic, 12) != 0 ||
	    (hdr.endianness != 0x04030201 && hdr.endianness != 0x01020304))
	{
		print_error("ERROR: Image header corrupt '%s'\n", filename);
		return true;
	}

	bool switch_endianness = false;
	if (hdr.endianness == 0x01020304)
	{
		switch_endianness = true;
		ktx_header_switch_endianness(&hdr);
	}

	if (hdr.gl_type != 0 || hdr.gl_format != 0 || hdr.gl_type_size != 1 ||
	    hdr.gl_base_internal_format != GL_RGBA)
	{
		print_error("ERROR: Image uses unsupported KTX format '%s'\n", filename);
		return true;
	}

	const format_entry* fmt = get_format(hdr.gl_internal_format);
	if (!fmt)
	{
		print_error("ERROR: Image uses unsupported KTX format '%s'\n", filename);
		return true;
	}

	// Skip over any key-value pairs
	file.seekg(hdr.bytes_of_key_value_data, std::ios::cur);
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return true;
	}

	// Read the length of the data and convert endianness
	uint32_t data_len;
	file.read(reinterpret_cast<char*>(&data_len), sizeof(data_len));
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return true;
	}

	if (switch_endianness)
	{
		data_len = reverse_bytes_u32(data_len);
	}

	// Read the data
	img.data.resize(data_len);
	file.read(reinterpret_cast<char*>(img.data.data()), data_len);
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		img.data.clear();
		return true;
	}

	img.block_x = fmt->x;
	img.block_y = fmt->y;
	img.block_z = fmt->z == 0 ? 1 : fmt->z;

	img.dim_x = hdr.pixel_width;
	img.dim_y = hdr.pixel_height;
	img.dim_z = hdr.pixel_depth == 0 ? 1 : hdr.pixel_depth;

	is_srgb = fmt->is_srgb;

	return false;
}

/**
 * @brief Store a KTX compressed image using a local store routine.
 *
 * @param img        The image data to store.
 * @param filename   The name of the file to save.
 * @param is_srgb    @c true if this is an sRGB image, @c false if linear.
 *
 * @return @c true on error, @c false otherwise.
 */
bool store_ktx_compressed_image(
	const astc_compressed_image& img,
	const char* filename,
	bool is_srgb
) {
	unsigned int fmt = get_format(img.block_x, img.block_y, img.block_z, is_srgb);

	ktx_header hdr;
	memcpy(hdr.magic, ktx_magic, 12);
	hdr.endianness = 0x04030201;
	hdr.gl_type = 0;
	hdr.gl_type_size = 1;
	hdr.gl_format = 0;
	hdr.gl_internal_format = fmt;
	hdr.gl_base_internal_format = GL_RGBA;
	hdr.pixel_width = img.dim_x;
	hdr.pixel_height = img.dim_y;
	hdr.pixel_depth = (img.dim_z == 1) ? 0 : img.dim_z;
	hdr.number_of_array_elements = 0;
	hdr.number_of_faces = 1;
	hdr.number_of_mipmap_levels = 1;
	hdr.bytes_of_key_value_data = 0;

#if defined(ASTCENC_BIG_ENDIAN)
	ktx_header_switch_endianness(&hdr);
#endif

	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (!file)
	{
		return true;
	}

	uint32_t data_len = static_cast<uint32_t>(img.data.size());
#if defined(ASTCENC_BIG_ENDIAN)
	data_len = reverse_bytes_u32(data_len);
#endif

	file.write(reinterpret_cast<const char*>(&hdr), sizeof(ktx_header));
	file.write(reinterpret_cast<const char*>(&data_len), sizeof(uint32_t));
	file.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());
	return file.fail();
}

/**
 * @brief Save a KTX uncompressed image using a local store routine.
 *
 * @param img        The source data for the image.
 * @param filename   The name of the file to save.
 * @param y_flip     Should the image be vertically flipped?
 *
 * @return @c true if the image saved OK, @c false on error.
 */
static bool store_ktx_uncompressed_image(
	const astcenc_image* img,
	const char* filename,
	int y_flip
) {
	size_t dim_x = img->dim_x;
	size_t dim_y = img->dim_y;
	size_t dim_z = img->dim_z;

	unsigned int bitness = img->data_type == ASTCENC_TYPE_U8 ? 8 : 16;
	unsigned int image_components = determine_image_components(img);

	// Size of the image data in bytes
	bool overflow { false };
	size_t image_bytes = astc::mul_safe(dim_x, dim_y, overflow);
	image_bytes = astc::mul_safe(image_bytes, dim_z, overflow);
	image_bytes = astc::mul_safe(image_bytes, image_components, overflow);
	image_bytes = astc::mul_safe(image_bytes, bitness / 8, overflow);
	if (overflow)
	{
		return false;
	}

	// Size of image data padded to a multiple of 4 bytes
	size_t image_write_bytes = (image_bytes + 3) & ~3;

	// The KTX imageSize field is a fixed 32-bit value, so check that the size_t
	/// value can be safely narrowed, and does not wrap when padded
	uint32_t image_bytes_field = static_cast<uint32_t>(image_bytes);
	if ((image_bytes_field != image_bytes) || (image_write_bytes < image_bytes))
	{
		return false;
	}

	ktx_header hdr;

	static const int gl_format_of_components[4] {
		GL_RED, GL_RG, GL_RGB, GL_RGBA
	};

	static const int gl_sized_format_of_components_ldr[4] {
		GL_R8, GL_RG8, GL_RGB8, GL_RGBA8
	};

	static const int gl_sized_format_of_components_hdr[4] {
		GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F
	};

	memcpy(hdr.magic, ktx_magic, 12);
	hdr.endianness = 0x04030201;
	hdr.gl_type = (bitness == 16) ? GL_HALF_FLOAT : GL_UNSIGNED_BYTE;
	hdr.gl_type_size = bitness / 8;
	hdr.gl_format = gl_format_of_components[image_components - 1];

	if (bitness == 16)
	{
		hdr.gl_internal_format = gl_sized_format_of_components_hdr[image_components - 1];
	}
	else
	{
		hdr.gl_internal_format = gl_sized_format_of_components_ldr[image_components - 1];
	}

	hdr.gl_base_internal_format = hdr.gl_format;
	hdr.pixel_width = static_cast<uint32_t>(dim_x);
	hdr.pixel_height = static_cast<uint32_t>(dim_y);
	hdr.pixel_depth = static_cast<uint32_t>((dim_z == 1) ? 0 : dim_z);
	hdr.number_of_array_elements = 0;
	hdr.number_of_faces = 1;
	hdr.number_of_mipmap_levels = 1;
	hdr.bytes_of_key_value_data = 0;

	// Collect image data to write
	std::vector<uint8_t> pixel_data8;
	std::vector<uint8_t*> row_data8;
	std::vector<uint8_t**> row_pointers8;

	std::vector<uint16_t> pixel_data16;
	std::vector<uint16_t*> row_data16;
	std::vector<uint16_t**> row_pointers16;

	if (bitness == 8)
	{
		row_pointers8.resize(dim_z);
		row_data8.resize(dim_y * dim_z);
		// + 3 to ensure this can be padded to be word aligned when written
		pixel_data8.resize(dim_x * dim_y * dim_z * image_components + 3);

		row_pointers8[0] = row_data8.data();
		row_pointers8[0][0] = pixel_data8.data();

		for (size_t z = 1; z < dim_z; z++)
		{
			row_pointers8[z] = row_pointers8[0] + dim_y * z;
			row_pointers8[z][0] = row_pointers8[0][0] + dim_x * dim_y * image_components * z;
		}

		for (size_t z = 0; z < dim_z; z++)
		{
			for (size_t y = 1; y < dim_y; y++)
			{
				row_pointers8[z][y] = row_pointers8[z][0] + dim_x * image_components * y;
			}
		}

		for (size_t z = 0; z < dim_z; z++)
		{
			uint8_t* data8 = static_cast<uint8_t*>(img->data[z]);
			for (size_t y = 0; y < dim_y; y++)
			{
				size_t mod_y = y_flip ? dim_y - y - 1 : y;
				switch (image_components)
				{
				case 1:  // One component, treated as Luminance
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers8[z][y][x] = data8[(4 * dim_x* mod_y) + (4 * x    )];
					}
					break;
				case 2:  // Two component, treated as Luminance-Alpha
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers8[z][y][2 * x    ] = data8[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers8[z][y][2 * x + 1] = data8[(4 * dim_x* mod_y) + (4 * x + 3)];
					}
					break;
				case 3:  // Three component, treated as RGB
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers8[z][y][3 * x    ] = data8[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers8[z][y][3 * x + 1] = data8[(4 * dim_x* mod_y) + (4 * x + 1)];
						row_pointers8[z][y][3 * x + 2] = data8[(4 * dim_x* mod_y) + (4 * x + 2)];
					}
					break;
				case 4:  // Four component, treated as RGBA
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers8[z][y][4 * x    ] = data8[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers8[z][y][4 * x + 1] = data8[(4 * dim_x* mod_y) + (4 * x + 1)];
						row_pointers8[z][y][4 * x + 2] = data8[(4 * dim_x* mod_y) + (4 * x + 2)];
						row_pointers8[z][y][4 * x + 3] = data8[(4 * dim_x* mod_y) + (4 * x + 3)];
					}
					break;
				}
			}
		}
	}
	else  // if bitness == 16
	{
		row_pointers16.resize(dim_z);
		row_data16.resize(dim_y * dim_z);
		// + 1 to ensure this can be padded to be word aligned when written
		pixel_data16.resize(dim_x * dim_y * dim_z * image_components + 1);

		row_pointers16[0] = row_data16.data();
		row_pointers16[0][0] = pixel_data16.data();

		for (size_t z = 1; z < dim_z; z++)
		{
			row_pointers16[z] = row_pointers16[0] + dim_y * z;
			row_pointers16[z][0] = row_pointers16[0][0] + dim_x * dim_y * image_components * z;
		}

		for (size_t z = 0; z < dim_z; z++)
		{
			for (size_t y = 1; y < dim_y; y++)
			{
				row_pointers16[z][y] = row_pointers16[z][0] + dim_x * image_components * y;
			}
		}

		for (size_t z = 0; z < dim_z; z++)
		{
			uint16_t* data16 = static_cast<uint16_t*>(img->data[z]);
			for (size_t y = 0; y < dim_y; y++)
			{
				size_t mod_y = y_flip ? dim_y - y - 1 : y;
				switch (image_components)
				{
				case 1:  // One component, treated as Luminance
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers16[z][y][x] = data16[(4 * dim_x* mod_y) + (4 * x    )];
					}
					break;
				case 2:  // Two component, treated as Luminance-Alpha
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers16[z][y][2 * x    ] = data16[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers16[z][y][2 * x + 1] = data16[(4 * dim_x* mod_y) + (4 * x + 3)];
					}
					break;
				case 3:  // Three component, treated as RGB
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers16[z][y][3 * x    ] = data16[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers16[z][y][3 * x + 1] = data16[(4 * dim_x* mod_y) + (4 * x + 1)];
						row_pointers16[z][y][3 * x + 2] = data16[(4 * dim_x* mod_y) + (4 * x + 2)];
					}
					break;
				case 4:  // Four component, treated as RGBA
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers16[z][y][4 * x    ] = data16[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers16[z][y][4 * x + 1] = data16[(4 * dim_x* mod_y) + (4 * x + 1)];
						row_pointers16[z][y][4 * x + 2] = data16[(4 * dim_x* mod_y) + (4 * x + 2)];
						row_pointers16[z][y][4 * x + 3] = data16[(4 * dim_x* mod_y) + (4 * x + 3)];
					}
					break;
				}
			}
		}
	}

	bool retval { true };
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (file)
	{
		void* dataptr = (bitness == 16) ?
			reinterpret_cast<void*>(row_pointers16[0][0]) :
			reinterpret_cast<void*>(row_pointers8[0][0]);

		file.write(reinterpret_cast<const char*>(&hdr), sizeof(ktx_header));
		file.write(reinterpret_cast<const char*>(&image_bytes_field), sizeof(image_bytes_field));
		file.write(reinterpret_cast<const char*>(dataptr), image_write_bytes);
		if (file.fail())
		{
			retval = false;
		}
	}
	else
	{
		retval = false;
	}

	return retval;
}

/*
	Loader for DDS files.

	Note that after the header, data are densely packed with no padding;
	in the case of multiple surfaces, they appear one after another in
	the file, again with no padding.

	This code is NOT endian-neutral.
*/
struct dds_pixelformat
{
	uint32_t size;				// structure size, set to 32.
	/*
	   flags bits are a combination of the following: 0x1 : Texture contains alpha data 0x2 : ---- (older files: texture contains alpha data, for Alpha-only texture) 0x4 : The fourcc field is valid,
	   indicating a compressed or DX10 texture format 0x40 : texture contains uncompressed RGB data 0x200 : ---- (YUV in older files) 0x20000 : Texture contains Luminance data (can be combined with
	   0x1 for Lum-Alpha) */
	uint32_t flags;
	uint32_t fourcc;			// "DX10" to indicate a DX10 format, "DXTn" for the DXT formats
	uint32_t rgbbitcount;		// number of bits per texel; up to 32 for non-DX10 formats.
	uint32_t rbitmask;			// bitmap indicating position of red/luminance color component
	uint32_t gbitmask;			// bitmap indicating position of green color component
	uint32_t bbitmask;			// bitmap indicating position of blue color component
	uint32_t abitmask;			// bitmap indicating position of alpha color component
};

struct dds_header
{
	uint32_t size;				// header size; must be exactly 124.
	/*
	   flag field is an OR or the following bits, that indicate fields containing valid data:
		1: caps/caps2/caps3/caps4 (set in all DDS files, ignore on read)
		2: height (set in all DDS files, ignore on read)
		4: width (set in all DDS files, ignore on read)
		8: pitch (for uncompressed texture)
		0x1000: the pixel format field (set in all DDS files, ignore on read)
		0x20000: mipmap count (for mipmapped textures with >1 level)
		0x80000: pitch (for compressed texture)
		0x800000: depth (for 3d textures)
	*/
	uint32_t flags;
	uint32_t height;
	uint32_t width;
	uint32_t pitch_or_linear_size;	// scanline pitch for uncompressed; total size in bytes for compressed
	uint32_t depth;
	uint32_t mipmapcount;
	// unused, set to 0
	uint32_t reserved1[11];
	dds_pixelformat ddspf;
	/*
	   caps field is an OR of the following values:
		8 : should be set for a file that contains more than 1 surface (ignore on read)
		0x400000 : should be set for a mipmapped texture
		0x1000 : should be set if the surface is a texture at all (all DDS files, ignore on read)
	*/
	uint32_t caps;
	/*
	   caps2 field is an OR of the following values:
		0x200 : texture is cubemap
		0x400 : +X face of cubemap is present
		0x800 : -X face of cubemap is present
		0x1000 : +Y face of cubemap is present
		0x2000 : -Y face of cubemap is present
		0x4000 : +Z face of cubemap is present
		0x8000 : -Z face of cubemap is present
		0x200000 : texture is a 3d texture.
	*/
	uint32_t caps2;
	// unused, set to 0
	uint32_t caps3;
	// unused, set to 0
	uint32_t caps4;
	// unused, set to 0
	uint32_t reserved2;
};

struct dds_header_dx10
{
	uint32_t dxgi_format;
	uint32_t resource_dimension;  // Set to 2 (1D tex), 3 (2D tex or cube), or 4 (3D tex)
	uint32_t misc_flag;           // Set to 4 if cubemap, else 0
	uint32_t array_size;          // Set to size of array if texture array; else 1
	uint32_t reserved;            // Set to 0.
};

#define DDS_MAGIC 0x20534444
#define DX10_MAGIC 0x30315844

/**
 * @brief Load an uncompressed DDS image using the local custom loader.
 *
 * @param      filename          The name of the file to load.
 * @param      y_flip            Should the image be vertically flipped?
 * @param[out] is_hdr            Is this an HDR image load?
 * @param[out] component_count   The number of components in the data.
 *
 * @return The loaded image data in a canonical 4 channel format, or @c nullptr on error.
 */
static astcenc_image_ptr load_dds_uncompressed_image(
	const char* filename,
	bool y_flip,
	bool& is_hdr,
	unsigned int& component_count
) {
	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if (!file)
	{
		print_error("ERROR: File open failed '%s'\n", filename);
		return nullptr;
	}

	// Read and check the DDS magic number
	uint32_t magic;
	file.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return nullptr;
	}

#if defined(ASTCENC_BIG_ENDIAN)
	magic = reverse_bytes_u32(magic);
#endif

	if (magic != DDS_MAGIC)
	{
		print_error("ERROR: Image header corrupt '%s'\n", filename);
		return nullptr;
	}

	// Validate that we can read the DDS header
	dds_header hdr;
	file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return nullptr;
	}

#if defined(ASTCENC_BIG_ENDIAN)
	// DDS header fields are all 32-bit words
	uint32_t* words = reinterpret_cast<uint32_t*>(&hdr);
	size_t word_count = sizeof(hdr) / sizeof(uint32_t);

	// Reverse all of them
	for (size_t i = 0; i < word_count; i++)
	{
		words[i] = reverse_bytes_u32(words[i]);
	}
#endif

	if (hdr.size != 124)
	{
		print_error("ERROR: Image header corrupt '%s'\n", filename);
		return nullptr;
	}

	int use_dx10_header = 0;
	if (hdr.ddspf.flags & 4)
	{
		if (hdr.ddspf.fourcc == DX10_MAGIC)
		{
			use_dx10_header = 1;
		}
		else
		{
			print_error("ERROR: Image uses unsupported DDS format '%s'\n", filename);
			return nullptr;
		}
	}

	dds_header_dx10 dx10_header;
	if (use_dx10_header)
	{
		file.read(reinterpret_cast<char*>(&dx10_header), sizeof(dx10_header));
		if (file.fail())
		{
			print_error("ERROR: File read failed '%s'\n", filename);
			return nullptr;
		}
	}

	size_t dim_x = hdr.width;
	size_t dim_y = hdr.height;
	size_t dim_z = (hdr.flags & 0x800000) ? hdr.depth : 1;

	// The bitcount that we will use internally in the codec
	unsigned int bitness = 0;

	// The bytes per component in the DDS file itself
	unsigned int bytes_per_component = 0;
	unsigned int components = 0;
	scanline_transfer copy_method = R8_TO_RGBA8;

	// figure out the format actually used in the DDS file.
	if (use_dx10_header)
	{
		// DX10 header present; use the DXGI format.
		#define DXGI_FORMAT_R32G32B32A32_FLOAT   2
		#define DXGI_FORMAT_R32G32B32_FLOAT      6
		#define DXGI_FORMAT_R16G16B16A16_FLOAT  10
		#define DXGI_FORMAT_R16G16B16A16_UNORM  11
		#define DXGI_FORMAT_R32G32_FLOAT        16
		#define DXGI_FORMAT_R8G8B8A8_UNORM      28
		#define DXGI_FORMAT_R16G16_FLOAT        34
		#define DXGI_FORMAT_R16G16_UNORM        35
		#define DXGI_FORMAT_R32_FLOAT           41
		#define DXGI_FORMAT_R8G8_UNORM          49
		#define DXGI_FORMAT_R16_FLOAT           54
		#define DXGI_FORMAT_R16_UNORM           56
		#define DXGI_FORMAT_R8_UNORM            61
		#define DXGI_FORMAT_B8G8R8A8_UNORM      86
		#define DXGI_FORMAT_B8G8R8X8_UNORM      87

		struct dxgi_params
		{
			unsigned int bitness;
			unsigned int bytes_per_component;
			unsigned int components;
			scanline_transfer copy_method;
			uint32_t dxgi_format_number;
		};

		static const dxgi_params format_params[] {
			{ 16, 4, 4, RGBA32F_TO_RGBA16F, DXGI_FORMAT_R32G32B32A32_FLOAT },
			{ 16, 4, 3,  RGB32F_TO_RGBA16F,    DXGI_FORMAT_R32G32B32_FLOAT },
			{ 16, 2, 4, RGBA16F_TO_RGBA16F, DXGI_FORMAT_R16G16B16A16_FLOAT },
			{ 16, 2, 4,  RGBA16_TO_RGBA16F, DXGI_FORMAT_R16G16B16A16_UNORM },
			{ 16, 4, 2,   RG32F_TO_RGBA16F,       DXGI_FORMAT_R32G32_FLOAT },
			{  8, 1, 4,     RGBA8_TO_RGBA8,     DXGI_FORMAT_R8G8B8A8_UNORM },
			{ 16, 2, 2,   RG16F_TO_RGBA16F,       DXGI_FORMAT_R16G16_FLOAT },
			{ 16, 2, 2,    RG16_TO_RGBA16F,       DXGI_FORMAT_R16G16_UNORM },
			{ 16, 4, 1,    R32F_TO_RGBA16F,          DXGI_FORMAT_R32_FLOAT },
			{  8, 1, 2,       RG8_TO_RGBA8,         DXGI_FORMAT_R8G8_UNORM },
			{ 16, 2, 1,    R16F_TO_RGBA16F,          DXGI_FORMAT_R16_FLOAT },
			{ 16, 2, 1,     R16_TO_RGBA16F,          DXGI_FORMAT_R16_UNORM },
			{  8, 1, 1,        R8_TO_RGBA8,           DXGI_FORMAT_R8_UNORM },
			{  8, 1, 4,     BGRA8_TO_RGBA8,     DXGI_FORMAT_B8G8R8A8_UNORM },
			{  8, 1, 4,      BGRX8_TO_RGBA8,    DXGI_FORMAT_B8G8R8X8_UNORM },
		};

		int dxgi_modes_supported = sizeof(format_params) / sizeof(format_params[0]);
		int did_select_format = 0;
		for (int i = 0; i < dxgi_modes_supported; i++)
		{
			if (dx10_header.dxgi_format == format_params[i].dxgi_format_number)
			{
				bitness = format_params[i].bitness;
				bytes_per_component = format_params[i].bytes_per_component;
				components = format_params[i].components;
				copy_method = format_params[i].copy_method;
				did_select_format = 1;
				break;
			}
		}

		if (!did_select_format)
		{
			print_error("ERROR: Image uses unsupported DDS format '%s'\n", filename);
			return nullptr;
		}
	}
	else
	{
		// No DX10 header present. Then try to match the bitcount and bitmask against
		// a set of prepared patterns.
		uint32_t flags = hdr.ddspf.flags;
		uint32_t bitcount = hdr.ddspf.rgbbitcount;
		uint32_t rmask = hdr.ddspf.rbitmask;
		uint32_t gmask = hdr.ddspf.gbitmask;
		uint32_t bmask = hdr.ddspf.bbitmask;
		uint32_t amask = hdr.ddspf.abitmask;

		// RGBA-unorm8
		if ((flags & 0x41) == 0x41 && bitcount == 32 && rmask == 0xFF && gmask == 0xFF00 && bmask == 0xFF0000 && amask == 0xFF000000)
		{
			bytes_per_component = 1;
			components = 4;
			copy_method = RGBA8_TO_RGBA8;
		}
		// BGRA-unorm8
		else if ((flags & 0x41) == 0x41 && bitcount == 32 && rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF && amask == 0xFF000000)
		{
			bytes_per_component = 1;
			components = 4;
			copy_method = BGRA8_TO_RGBA8;
		}
		// RGBX-unorm8
		else if ((flags & 0x40) && bitcount == 32 && rmask == 0xFF && gmask == 0xFF00 && bmask == 0xFF0000)
		{
			bytes_per_component = 1;
			components = 4;
			copy_method = RGBX8_TO_RGBA8;
		}
		// BGRX-unorm8
		else if ((flags & 0x40) && bitcount == 32 && rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF)
		{
			bytes_per_component = 1;
			components = 4;
			copy_method = BGRX8_TO_RGBA8;
		}
		// RGB-unorm8
		else if ((flags & 0x40) && bitcount == 24 && rmask == 0xFF && gmask == 0xFF00 && bmask == 0xFF0000)
		{
			bytes_per_component = 1;
			components = 3;
			copy_method = RGB8_TO_RGBA8;
		}
		// BGR-unorm8
		else if ((flags & 0x40) && bitcount == 24 && rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF)
		{
			bytes_per_component = 1;
			components = 3;
			copy_method = BGR8_TO_RGBA8;
		}
		// RG-unorm16
		else if ((flags & 0x40) && bitcount == 16 && rmask == 0xFFFF && gmask == 0xFFFF0000)
		{
			bytes_per_component = 2;
			components = 2;
			copy_method = RG16_TO_RGBA16F;
		}
		// A8L8
		else if ((flags & 0x20001) == 0x20001 && bitcount == 16 && rmask == 0xFF && amask == 0xFF00)
		{
			bytes_per_component = 1;
			components = 2;
			copy_method = LA8_TO_RGBA8;
		}
		// L8
		else if ((flags & 0x20000) && bitcount == 8 && rmask == 0xFF)
		{
			bytes_per_component = 1;
			components = 1;
			copy_method = L8_TO_RGBA8;
		}
		// L16
		else if ((flags & 0x20000) && bitcount == 16 && rmask == 0xFFFF)
		{
			bytes_per_component = 2;
			components = 1;
			copy_method = L16_TO_RGBA16F;
		}
		else
		{
			print_error("ERROR: Image uses unsupported DDS format '%s'\n", filename);
			return nullptr;
		}

		bitness = bytes_per_component * 8;
	}

	// Compute surface size, checking for overflow caused by bad user-defined sizes
	// These values are trusted and cannot overflow
	size_t bytes_per_pixel = bytes_per_component * components;

	// These values are not and can overflow
	bool overflow { false };
	size_t bytes_per_row = astc::mul_safe(bytes_per_pixel, dim_x, overflow);
	size_t bytes_per_plane = astc::mul_safe(bytes_per_row, dim_y, overflow);
	size_t bytes_per_image = astc::mul_safe(bytes_per_plane, dim_z, overflow);

	// Also verify that our output plane allocation does not overflow because
	// this always uses 4 components which can be more than the input file used
	size_t plane_texels = astc::mul_safe(dim_x, dim_y, overflow);
	astc::mul_safe(plane_texels, 4, overflow);

	if (overflow)
	{
		print_error("ERROR: Image header corrupt '%s'\n", filename);
		return nullptr;
	}

	std::unique_ptr<uint8_t[]> buf;
	try
	{
		buf = std::make_unique<uint8_t[]>(bytes_per_image);
	}
	catch (const std::bad_alloc &e)
	{
		ASTCENC_UNUSED(e);
		print_error("ERROR: Image memory allocation failed '%s'\n", filename);
		return nullptr;
	}

	file.read(reinterpret_cast<char*>(buf.get()), bytes_per_image);
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return nullptr;
	}

	// Transfer data from the surface to our own image data structure
	astcenc_image_ptr astc_img;
	try
	{
		astc_img = alloc_image(
			bitness,
			// TODO: Casts are ugly, should fix astcenc_image to accept size_t
			static_cast<unsigned int>(dim_x),
			static_cast<unsigned int>(dim_y),
			static_cast<unsigned int>(dim_z));
	}
	catch (const std::bad_alloc &e)
	{
		ASTCENC_UNUSED(e);
		print_error("ERROR: Image memory allocation failed '%s'\n", filename);
		return nullptr;
	}

	// TODO: Change astenc_image struct to store size_t rather than unsigned
	// int then make dim_x/y/z size_t at the start of the function. This is an
	// API break, so needs to wait until we make a major version.
	size_t dim_y_sz = dim_y;
	size_t dim_z_sz = dim_z;

	for (size_t z = 0; z < dim_z_sz; z++)
	{
		for (size_t y = 0; y < dim_y_sz; y++)
		{
			size_t mod_y = y_flip ? dim_y_sz - y - 1 : y;
			void* dst;

			if (astc_img->data_type == ASTCENC_TYPE_U8)
			{
				uint8_t* data8 = static_cast<uint8_t*>(astc_img->data[z]);
				dst = static_cast<void*>(&data8[4 * dim_x * mod_y]);
			}
			else // if (astc_img->data_type == ASTCENC_TYPE_F16)
			{
				assert(astc_img->data_type == ASTCENC_TYPE_F16);
				uint16_t* data16 = static_cast<uint16_t*>(astc_img->data[z]);
				dst = static_cast<void*>(&data16[4 * dim_x * mod_y]);
			}

			uint8_t *src = buf.get() + (z * bytes_per_plane) + (y * bytes_per_row);
			copy_scanline(dst, src, dim_x, copy_method);
		}
	}

	is_hdr = bitness >= 16;
	component_count = components;
	return astc_img;
}

/**
 * @brief Save a DDS uncompressed image using a local store routine.
 *
 * @param img        The source data for the image.
 * @param filename   The name of the file to save.
 * @param y_flip     Should the image be vertically flipped?
 *
 * @return @c true if the image saved OK, @c false on error.
 */
static bool store_dds_uncompressed_image(
	const astcenc_image* img,
	const char* filename,
	int y_flip
) {
	size_t dim_x = img->dim_x;
	size_t dim_y = img->dim_y;
	size_t dim_z = img->dim_z;

	unsigned int bitness = img->data_type == ASTCENC_TYPE_U8 ? 8 : 16;
	unsigned int image_components = (bitness == 16) ? 4 : determine_image_components(img);

	// Size of the image data in bytes
	bool overflow { false };
	size_t image_bytes = astc::mul_safe(dim_x, dim_y, overflow);
	image_bytes = astc::mul_safe(image_bytes, dim_z, overflow);
	image_bytes = astc::mul_safe(image_bytes, image_components, overflow);
	image_bytes = astc::mul_safe(image_bytes, bitness / 8, overflow);
	if (overflow)
	{
		return false;
	}

	// Must be smaller than image_bytes, so no need to check overflow here
	size_t pitch_bytes = dim_x * image_components * (bitness / 8);

	// The pitch_or_linear_size field is a fixed 32-bit value, so the size_t used
	// for buffer sizing must be narrowed before it is serialized into the file.
	uint32_t pitch_bytes_field = static_cast<uint32_t>(pitch_bytes);
	if (pitch_bytes_field != pitch_bytes)
	{
		return false;
	}

	// DDS-pixel-format structures to use when storing LDR image with 1,2,3 or 4 components.
	static const dds_pixelformat format_of_image_components[4] =
	{
		{32, 0x20000, 0, 8, 0xFF, 0, 0, 0},	// luminance
		{32, 0x20001, 0, 16, 0xFF, 0, 0, 0xFF00},	// L8A8
		{32, 0x40, 0, 24, 0xFF, 0xFF00, 0xFF0000, 0},	// RGB8
		{32, 0x41, 0, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000}	// RGBA8
	};

	// DDS-pixel-format structures to use when storing HDR image.
	static const dds_pixelformat dxt10_diverter =
	{
		32, 4, DX10_MAGIC, 0, 0, 0, 0, 0
	};

	// Header handling; will write:
	// * DDS magic value
	// * DDS header
	// * DDS DX10 header, if the file is floating-point
	// * pixel data

	// Main header data
	dds_header hdr;
	hdr.size = 124;
	hdr.flags = 0x100F | (dim_z > 1 ? 0x800000 : 0);
	hdr.height = static_cast<uint32_t>(dim_y);
	hdr.width = static_cast<uint32_t>(dim_x);
	hdr.pitch_or_linear_size = pitch_bytes_field;
	hdr.depth = static_cast<uint32_t>(dim_z);
	hdr.mipmapcount = 1;
	for (unsigned int i = 0; i < 11; i++)
	{
		hdr.reserved1[i] = 0;
	}
	hdr.caps = 0x1000;
	hdr.caps2 = (dim_z > 1) ? 0x200000 : 0;
	hdr.caps3 = 0;
	hdr.caps4 = 0;
	hdr.reserved2 = 0;

	// Pixel-format data
	if (bitness == 8)
	{
		hdr.ddspf = format_of_image_components[image_components - 1];
	}
	else
	{
		hdr.ddspf = dxt10_diverter;
	}

	// DX10 data
	dds_header_dx10 dx10;
	dx10.dxgi_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	dx10.resource_dimension = (dim_z > 1) ? 4 : 3;
	dx10.misc_flag = 0;
	dx10.array_size = 1;
	dx10.reserved = 0;

	// Collect image data to write
	std::vector<uint8_t> pixel_data8;
	std::vector<uint8_t*> row_data8;
	std::vector<uint8_t**> row_pointers8;

	std::vector<uint16_t> pixel_data16;
	std::vector<uint16_t*> row_data16;
	std::vector<uint16_t**> row_pointers16;

	if (bitness == 8)
	{
		row_pointers8.resize(dim_z);
		row_data8.resize(dim_y * dim_z);
		pixel_data8.resize(dim_x * dim_y * dim_z * image_components);

		row_pointers8[0] = row_data8.data();
		row_pointers8[0][0] = pixel_data8.data();

		for (size_t z = 1; z < dim_z; z++)
		{
			row_pointers8[z] = row_pointers8[0] + dim_y * z;
			row_pointers8[z][0] = row_pointers8[0][0] + dim_x * dim_y * image_components * z;
		}

		for (size_t z = 0; z < dim_z; z++)
		{
			for (size_t y = 1; y < dim_y; y++)
			{
				row_pointers8[z][y] = row_pointers8[z][0] + dim_x * image_components * y;
			}
		}

		for (size_t z = 0; z < dim_z; z++)
		{
			uint8_t* data8 = static_cast<uint8_t*>(img->data[z]);

			for (size_t y = 0; y < dim_y; y++)
			{
				size_t mod_y = y_flip ? dim_y - y - 1 : y;
				switch (image_components)
				{
				case 1:  // One component, treated as Luminance
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers8[z][y][x] = data8[(4 * dim_x* mod_y) + (4 * x    )];
					}
					break;
				case 2:  // Two component, treated as Luminance-Alpha
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers8[z][y][2 * x    ] = data8[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers8[z][y][2 * x + 1] = data8[(4 * dim_x* mod_y) + (4 * x + 3)];
					}
					break;
				case 3:  // Three component, treated as RGB
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers8[z][y][3 * x    ] = data8[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers8[z][y][3 * x + 1] = data8[(4 * dim_x* mod_y) + (4 * x + 1)];
						row_pointers8[z][y][3 * x + 2] = data8[(4 * dim_x* mod_y) + (4 * x + 2)];
					}
					break;
				case 4:  // Four component, treated as RGBA
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers8[z][y][4 * x    ] = data8[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers8[z][y][4 * x + 1] = data8[(4 * dim_x* mod_y) + (4 * x + 1)];
						row_pointers8[z][y][4 * x + 2] = data8[(4 * dim_x* mod_y) + (4 * x + 2)];
						row_pointers8[z][y][4 * x + 3] = data8[(4 * dim_x* mod_y) + (4 * x + 3)];
					}
					break;
				}
			}
		}
	}
	else  // if bitness == 16
	{
		row_pointers16.resize(dim_z);
		row_data16.resize(dim_y * dim_z);
		pixel_data16.resize(dim_x * dim_y * dim_z * image_components);

		row_pointers16[0] = row_data16.data();
		row_pointers16[0][0] = pixel_data16.data();

		for (size_t z = 1; z < dim_z; z++)
		{
			row_pointers16[z] = row_pointers16[0] + dim_y * z;
			row_pointers16[z][0] = row_pointers16[0][0] + dim_x * dim_y * image_components * z;
		}

		for (size_t z = 0; z < dim_z; z++)
		{
			for (size_t y = 1; y < dim_y; y++)
			{
				row_pointers16[z][y] = row_pointers16[z][0] + dim_x * image_components * y;
			}
		}

		for (size_t z = 0; z < dim_z; z++)
		{
			uint16_t* data16 = static_cast<uint16_t*>(img->data[z]);

			for (size_t y = 0; y < dim_y; y++)
			{
				size_t mod_y = y_flip ? dim_y - y - 1: y;
				switch (image_components)
				{
				case 1:  // One component, treated as Luminance
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers16[z][y][x] = data16[(4 * dim_x* mod_y) + (4 * x    )];
					}
					break;
				case 2:  // Two component, treated as Luminance-Alpha
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers16[z][y][2 * x    ] = data16[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers16[z][y][2 * x + 1] = data16[(4 * dim_x* mod_y) + (4 * x + 3)];
					}
					break;
				case 3:  // Three component, treated as RGB
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers16[z][y][3 * x    ] = data16[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers16[z][y][3 * x + 1] = data16[(4 * dim_x* mod_y) + (4 * x + 1)];
						row_pointers16[z][y][3 * x + 2] = data16[(4 * dim_x* mod_y) + (4 * x + 2)];
					}
					break;
				case 4:  // Four-component, treated as RGBA
					for (size_t x = 0; x < dim_x; x++)
					{
						row_pointers16[z][y][4 * x    ] = data16[(4 * dim_x* mod_y) + (4 * x    )];
						row_pointers16[z][y][4 * x + 1] = data16[(4 * dim_x* mod_y) + (4 * x + 1)];
						row_pointers16[z][y][4 * x + 2] = data16[(4 * dim_x* mod_y) + (4 * x + 2)];
						row_pointers16[z][y][4 * x + 3] = data16[(4 * dim_x* mod_y) + (4 * x + 3)];
					}
					break;
				}
			}
		}
	}

	bool retval { true };

	uint32_t dds_magic = DDS_MAGIC;

	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (file)
	{
		void *dataptr = (bitness == 16) ?
			reinterpret_cast<void*>(row_pointers16[0][0]) :
			reinterpret_cast<void*>(row_pointers8[0][0]);

		file.write(reinterpret_cast<const char*>(&dds_magic), sizeof(dds_magic));
		file.write(reinterpret_cast<const char*>(&hdr), sizeof(dds_header));
		if (bitness > 8)
		{
			file.write(reinterpret_cast<const char*>(&dx10), sizeof(dx10));
		}

		file.write(reinterpret_cast<const char*>(dataptr), image_bytes);
		if (file.fail())
		{
			retval = false;
		}
	}
	else
	{
		retval = false;
	}

	return retval;
}

/**
 * @brief Image loader function pointer.
 */
using image_loader = astcenc_image_ptr(*)(const char*, bool, bool&, unsigned int&);

/**
 * @brief Specs for an image loader function.
 */
struct loader_specs
{
	const char* ending1;
	const char* ending2;
	const image_loader loader_func;
};

/**
 * @brief Supported uncompressed image load functions, and their associated file extensions.
 */
const loader_specs loader_descs[] {
	// LDR formats
	{  ".png",  ".PNG", load_png_with_wuffs},
	// HDR formats
	{  ".exr",  ".EXR", load_image_with_tinyexr },
	// Container formats
	{  ".ktx",  ".KTX", load_ktx_uncompressed_image },
	{  ".dds",  ".DDS", load_dds_uncompressed_image },
	// Generic catch all; this one must be last in the list
	{ nullptr, nullptr, load_image_with_stb }
};

static const size_t loader_descr_count = sizeof(loader_descs) / sizeof(loader_descs[0]);

/**
 * @brief Image storer function pointer.
 */
using image_storer = bool(*)(const astcenc_image*, const char*, int y_flip);

/**
 * @brief Specs for an image storer function.
 */
struct storer_specs
{
	const char *ending1;
	const char *ending2;
	int enforced_bitness;
	image_storer storer_func;
};

/**
 * @brief Supported uncompressed image store functions, and their associated file extensions.
 */
const storer_specs storer_descs[] {
	// LDR formats
	{ ".bmp", ".BMP",  8, store_bmp_image_with_stb },
	{ ".png", ".PNG",  8, store_png_image_with_stb },
	{ ".tga", ".TGA",  8, store_tga_image_with_stb },
	// HDR formats
	{ ".exr", ".EXR", 16, store_exr_image_with_tinyexr },
	{ ".hdr", ".HDR", 16, store_hdr_image_with_stb },
	// Container formats
	{ ".dds", ".DDS",  0, store_dds_uncompressed_image },
	{ ".ktx", ".KTX",  0, store_ktx_uncompressed_image }
};

static const size_t storer_descr_count = sizeof(storer_descs) / sizeof(storer_descs[0]);

/* See header for documentation. */
int get_output_filename_enforced_bitness(
	const char* filename
) {
	const char *eptr = strrchr(filename, '.');
	if (!eptr)
	{
		return 0;
	}

	for (size_t i = 0; i < storer_descr_count; i++)
	{
		if (strcmp(eptr, storer_descs[i].ending1) == 0
		 || strcmp(eptr, storer_descs[i].ending2) == 0)
		{
			return storer_descs[i].enforced_bitness;
		}
	}

	return -1;
}

/* See header for documentation. */
astcenc_image_ptr load_ncimage(
	const char* filename,
	bool y_flip,
	bool& is_hdr,
	unsigned int& component_count
) {
	// Get the file extension
	const char* eptr = strrchr(filename, '.');
	if (!eptr)
	{
		eptr = filename;
	}

	// Scan through descriptors until a matching loader is found
	image_loader loader { nullptr };
	for (size_t i = 0; i < loader_descr_count; i++)
	{
		if (loader_descs[i].ending1 == nullptr
			|| strcmp(eptr, loader_descs[i].ending1) == 0
			|| strcmp(eptr, loader_descs[i].ending2) == 0)
		{
			loader = loader_descs[i].loader_func;
			break;
		}
	}

	// We must always match a loader - stb_image provides a generic handler
	assert(loader);

	try
	{
		return loader(filename, y_flip, is_hdr, component_count);
	}
	catch (const std::bad_alloc &e)
	{
		ASTCENC_UNUSED(e);
		print_error("ERROR: Image memory allocation failed '%s'\n", filename);
		return nullptr;
	}
}

/* See header for documentation. */
bool store_ncimage(
	const astcenc_image* output_image,
	const char* filename,
	int y_flip
) {
	const char* eptr = strrchr(filename, '.');
	if (!eptr)
	{
		eptr = ".ktx"; // use KTX file format if we don't have an ending.
	}

	// Scan through descriptors until a matching storer is found
	image_storer storer { nullptr };
	for (size_t i = 0; i < storer_descr_count; i++)
	{
		if (strcmp(eptr, storer_descs[i].ending1) == 0
		 || strcmp(eptr, storer_descs[i].ending2) == 0)
		{
			storer = storer_descs[i].storer_func;
			break;
		}
	}

	// Should never fail this, get_output_filename_enforced_bitness should have
	// acted as a preflight check
	if (!storer)
	{
		return false;
	}

	return storer(output_image, filename, y_flip);
}

/* ============================================================================
	ASTC compressed file loading
============================================================================ */
struct astc_header
{
	uint8_t magic[4];
	uint8_t block_x;
	uint8_t block_y;
	uint8_t block_z;
	uint8_t dim_x[3];  // Dims = dim[0] + (dim[1] << 8) + (dim[2] << 16)
	uint8_t dim_y[3];  // Sizes are in texels
	uint8_t dim_z[3];  // Block count is inferred
};

static const uint32_t ASTC_MAGIC_ID = 0x5CA1AB13;

static unsigned int unpack_bytes(
	uint8_t a,
	uint8_t b,
	uint8_t c,
	uint8_t d
) {
	return (static_cast<unsigned int>(a)      ) +
	       (static_cast<unsigned int>(b) <<  8) +
	       (static_cast<unsigned int>(c) << 16) +
	       (static_cast<unsigned int>(d) << 24);
}

/* See header for documentation. */
int load_cimage(
	const char* filename,
	astc_compressed_image& img
) {
	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if (!file)
	{
		print_error("ERROR: File open failed '%s'\n", filename);
		return 1;
	}

	astc_header hdr;
	file.read(reinterpret_cast<char*>(&hdr), sizeof(astc_header));
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		return 1;
	}

	unsigned int magicval = unpack_bytes(hdr.magic[0], hdr.magic[1], hdr.magic[2], hdr.magic[3]);
	if (magicval != ASTC_MAGIC_ID)
	{
		print_error("ERROR: Image header corrupt '%s'\n", filename);
		return 1;
	}

	// Ensure these are not zero to avoid div by zero
	size_t block_x = astc::max(static_cast<unsigned int>(hdr.block_x), 1u);
	size_t block_y = astc::max(static_cast<unsigned int>(hdr.block_y), 1u);
	size_t block_z = astc::max(static_cast<unsigned int>(hdr.block_z), 1u);

	size_t dim_x = unpack_bytes(hdr.dim_x[0], hdr.dim_x[1], hdr.dim_x[2], 0);
	size_t dim_y = unpack_bytes(hdr.dim_y[0], hdr.dim_y[1], hdr.dim_y[2], 0);
	size_t dim_z = unpack_bytes(hdr.dim_z[0], hdr.dim_z[1], hdr.dim_z[2], 0);

	if (dim_x == 0 || dim_y == 0 || dim_z == 0)
	{
		print_error("ERROR: Image header corrupt '%s'\n", filename);
		return 1;
	}

	// These cannot overflow as dim_* values are limited to 2^24 - 1
	size_t blocks_x = (dim_x + block_x - 1) / block_x;
	size_t blocks_y = (dim_y + block_y - 1) / block_y;
	size_t blocks_z = (dim_z + block_z - 1) / block_z;

	// This can overflow if dim_* sizes are large
	bool overflow { false };

	size_t data_size = astc::mul_safe(blocks_x, blocks_y, overflow);
	data_size = astc::mul_safe(data_size, blocks_z, overflow);
	data_size = astc::mul_safe(data_size, 16, overflow);

	if (overflow)
	{
		print_error("ERROR: Image header corrupt '%s'\n", filename);
		return 1;
	}

	// Allocation may fail if image is suspiciously large
	try
	{
		img.data.resize(data_size);
	}
	catch (const std::bad_alloc &e)
	{
		ASTCENC_UNUSED(e);
		print_error("ERROR: Image memory allocation failed '%s'\n", filename);
		return 1;
	}

	file.read(reinterpret_cast<char*>(img.data.data()), data_size);
	if (file.fail())
	{
		print_error("ERROR: File read failed '%s'\n", filename);
		img.data.clear();
		return 1;
	}

	// Casts are safe - we know individual values are small enough
	img.block_x = static_cast<unsigned int>(block_x);
	img.block_y = static_cast<unsigned int>(block_y);
	img.block_z = static_cast<unsigned int>(block_z);

	img.dim_x = static_cast<unsigned int>(dim_x);
	img.dim_y = static_cast<unsigned int>(dim_y);
	img.dim_z = static_cast<unsigned int>(dim_z);

	return 0;
}

/* See header for documentation. */
int store_cimage(
	const astc_compressed_image& img,
	const char* filename
) {
	astc_header hdr;
	hdr.magic[0] =  ASTC_MAGIC_ID        & 0xFF;
	hdr.magic[1] = (ASTC_MAGIC_ID >>  8) & 0xFF;
	hdr.magic[2] = (ASTC_MAGIC_ID >> 16) & 0xFF;
	hdr.magic[3] = (ASTC_MAGIC_ID >> 24) & 0xFF;

	hdr.block_x = static_cast<uint8_t>(img.block_x);
	hdr.block_y = static_cast<uint8_t>(img.block_y);
	hdr.block_z = static_cast<uint8_t>(img.block_z);

	hdr.dim_x[0] =  img.dim_x        & 0xFF;
	hdr.dim_x[1] = (img.dim_x >>  8) & 0xFF;
	hdr.dim_x[2] = (img.dim_x >> 16) & 0xFF;

	hdr.dim_y[0] =  img.dim_y       & 0xFF;
	hdr.dim_y[1] = (img.dim_y >>  8) & 0xFF;
	hdr.dim_y[2] = (img.dim_y >> 16) & 0xFF;

	hdr.dim_z[0] =  img.dim_z        & 0xFF;
	hdr.dim_z[1] = (img.dim_z >>  8) & 0xFF;
	hdr.dim_z[2] = (img.dim_z >> 16) & 0xFF;

	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (!file)
	{
		print_error("ERROR: File open failed '%s'\n", filename);
		return 1;
	}

	file.write(reinterpret_cast<char*>(&hdr), sizeof(astc_header));
	file.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());
	return 0;
}
