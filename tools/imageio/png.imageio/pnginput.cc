// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2022 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

/**
 * @internal
 * @~English
 * @file
 *
 * @brief ImageInput from PNG format files.
 *
 * @author Mark Callow
 */

#include "imageio.h"

#include <array>
#include <iterator>
#include <sstream>
#include <stdexcept>

#include "imageio_utility.h"
#include "lodepng.h"
#include <KHR/khr_df.h>
#include "dfd.h"

class PngInput final : public ImageInput {
  public:
    PngInput() : ImageInput("png") {}
    virtual ~PngInput() { close(); }
    virtual void open(ImageSpec& newspec) override;
    virtual void close() override {
        decodingBegun = false;
    }

    virtual void readImage(void* bufferOut, size_t bufferByteCount,
                           uint32_t subimage, uint32_t miplevel,
                           const FormatDescriptor& targetFormat) override;

    /// Read a single scanline (all channels) of native data into contiguous
    /// memory.
    virtual void readNativeScanline(void* /*buffer*/,  size_t /*bufferByteCount*/,
                                    uint32_t /*y*/, uint32_t /*z*/,
                                    uint32_t /*subimage*/, uint32_t /*miplevel*/) override { };

  protected:
    void readHeader();
    void slurp();

    std::vector<char> pngBuffer;
    lodepng::State state;
    void* pIdat;
    size_t idatsize;
    bool colorConvert = false;
    uint32_t nextScanline = 0;
    bool decodingBegun = false;
};

ImageInput*
pngInputCreate()
{
    return new PngInput;
}

const char* pngInputExtensions[] = { "png", nullptr };

void
PngInput::open(ImageSpec& newspec)
{
    assert(isp != nullptr && "ImageInput not properly opened");

    readHeader();
    newspec = spec();
    nextScanline = 0;
}


void PngInput::slurp()
{
    size_t pngByteLength;

    isp->seekg(0, isp->end);
    pngByteLength = isp->tellg();
    isp->seekg(0, isp->beg);

    pngBuffer.resize(pngByteLength);
    isp->read(pngBuffer.data(), pngByteLength);
}


void
PngInput::readHeader()
{
    // Unfortunately LoadPNG doesn't believe in stdio. The functions
    // we need either read from memory or take a file name. To avoid
    // a potentially unnecessary slurp of the whole file check the
    // signature ourselves.
    uint8_t pngsig[8] = {
       0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
    };
    uint8_t filesig[sizeof(pngsig)];
    isp->read((char *)filesig, sizeof(pngsig));
    if (isp->fail())
       throwOnReadFailure();
    if (memcmp(filesig, pngsig, sizeof(pngsig)))
       throw different_format();

    // It's a PNG file.

    isp->seekg(0, isp->beg);
    // Slurp it into memory so we can use lodepng_inspect, to determine
    // the data type, and lodepng_chunk_find.
    //
    // Why no special case for when we've already read the file into a
    // stringstream (i.e. buffer.get() == isp)? Because the only way to access
    // such data is to call stringstream::str() which makes a copy. So treat
    // everything the same. For the same reason we slurp into a vector not
    // a stringstream here.
    slurp();

    unsigned int lodepngError;
    uint32_t w, h;
    lodepngError = lodepng_decode_chunks(&pIdat, &idatsize, &w, &h, &state,
                                         (const uint8_t*)pngBuffer.data(),
                                         pngBuffer.size());
    if (lodepngError) {
        throw std::runtime_error(
            fmt::format("PNG decode chunks error: {}.",
                        lodepng_error_text(lodepngError))
        );
    }

    // Tell the decoder to produce the same color type as the file. Exceptions
    // to this are made later.
    lodepng_color_mode_copy(&state.info_raw, &state.info_png.color);

    uint32_t componentCount = 0;
    uint32_t bitDepth = state.info_png.color.bitdepth;
    // Initialisation here keeps compilers happy in the LCT_MAX_OCTET cases.
    khr_df_model_e colorModel = KHR_DF_MODEL_RGBSDA;

    switch (state.info_png.color.colortype) {
      case LCT_GREY:
        if (state.info_png.color.key_defined) {
            state.info_raw.colortype = LCT_GREY_ALPHA;
            componentCount = 2;
            colorModel = KHR_DF_MODEL_YUVSDA;
        } else {
            componentCount = 1;
            colorModel = KHR_DF_MODEL_YUVSDA;
        }
        break;
      case LCT_RGB:
        if (state.info_png.color.key_defined) {
            state.info_raw.colortype = LCT_RGBA;
            componentCount = 4;
            colorModel = KHR_DF_MODEL_RGBSDA;
        } else {
            componentCount = 3;
            colorModel = KHR_DF_MODEL_RGBSDA;
        }
        break;
      case LCT_PALETTE: {
        // color.key_defined is not set for paletted. tRNS info is written
        // directly into the palette. To determine the colortype to expand to
        // here we need to check if there is a tRNS chunk.
	    const unsigned char *pTrnsChunk = nullptr;
        // 1st chunk after header
        const unsigned char* pFirstChunk = (unsigned char*)&pngBuffer[33];
        pTrnsChunk = lodepng_chunk_find_const(pFirstChunk,
                                (unsigned char*)&pngBuffer[pngBuffer.size()-1],
                                "tRNS");
        if (pTrnsChunk) {
            state.info_raw.colortype = LCT_RGBA;
            componentCount = 4;
            colorModel = KHR_DF_MODEL_RGBSDA;
        } else {
            state.info_raw.colortype = LCT_RGB;
            componentCount = 3;
            colorModel = KHR_DF_MODEL_RGBSDA;
        }
        // There are no paletted texture formats, except an ancient one in
        // OpenGL ES 1 & 2 so, rather than complicate the users of imageio
        // with handling for them, cause them to be expanded to 8 bits by
        // this reader and issue a warning.
        if (state.info_png.color.bitdepth < 8) {
            bitDepth = 8; // This value will be set in the ImageSpec and
                          // eventually passed back to readImage().
        }
        fwarning(fmt::format("Expanding {}-bit paletted image to {}",
                state.info_png.color.bitdepth,
                state.info_raw.colortype == LCT_RGBA ? "R8G8B8A8" : "R8G8B8"));
        }
        break;
      case LCT_GREY_ALPHA:
        componentCount = 2;
        colorModel = KHR_DF_MODEL_YUVSDA;
        break;
      case LCT_RGBA:
        colorModel = KHR_DF_MODEL_RGBSDA;
        componentCount = 4;
        break;
      case LCT_MAX_OCTET_VALUE:
        break;
    }

    ImageInputFormatType formatType;
    switch (state.info_png.color.colortype) {
    case LCT_GREY:
        formatType = ImageInputFormatType::png_l;
        break;
    case LCT_GREY_ALPHA:
        formatType = ImageInputFormatType::png_la;
        break;
    case LCT_RGB:
        formatType = ImageInputFormatType::png_rgb;
        break;
    case LCT_RGBA:
        formatType = ImageInputFormatType::png_rgba;
        break;
    case LCT_PALETTE:
        formatType = ImageInputFormatType::png_rgba;
        break;
    case LCT_MAX_OCTET_VALUE:
        break;
    }

    images.emplace_back(ImageSpec(w, h, 1, componentCount,
                            bitDepth,
                            static_cast<khr_df_sample_datatype_qualifiers_e>(0),
                            KHR_DF_TRANSFER_UNSPECIFIED,
                            // PNG spec. says BT.709 primaries are a
                            // reasonable default.
                            KHR_DF_PRIMARIES_BT709,
                            colorModel),
                        formatType);

    // This is ugly. FIXME:
    FormatDescriptor& format = const_cast<FormatDescriptor&>(spec().format());
    if (state.info_png.iccp_defined) {
        format.setPrimaries(KHR_DF_PRIMARIES_UNSPECIFIED);
        format.setTransfer(KHR_DF_TRANSFER_UNSPECIFIED);
        format.extended.iccProfile.name =  state.info_png.iccp_name;
        format.extended.iccProfile.profile.resize(state.info_png.iccp_profile_size);
        format.extended.iccProfile.profile.insert(
               format.extended.iccProfile.profile.begin(),
               state.info_png.iccp_profile,
               &state.info_png.iccp_profile[state.info_png.iccp_profile_size]);
        if (format.extended.iccProfile.name == "ITUR_2100_PQ_FULL") {
            format.setPrimaries(KHR_DF_PRIMARIES_BT2020);
            format.setTransfer(KHR_DF_TRANSFER_PQ_EOTF);
        }
    } else if (state.info_png.srgb_defined) {
        // srgb_intent is a guide for the user/application when applying
        // a color transform during rendering, especially when
        // gamut mapping. It does not affect the meaning or value
        // of the image pixels so there is nothing to do here.
        format.setTransfer(KHR_DF_TRANSFER_SRGB);
        format.setPrimaries(KHR_DF_PRIMARIES_SRGB);
    } else if (state.info_png.gama_defined) {
        format.setTransfer(KHR_DF_TRANSFER_UNSPECIFIED);
        // The value in the gAMA chunk is the exponent of the power curve
        // used for encoding the image, i.e. the OETF, * 100000.
        format.extended.oeGamma = (float)state.info_png.gama_gamma / 100000;
    } else {
        format.setTransfer(KHR_DF_TRANSFER_UNSPECIFIED);
    }

    if (state.info_png.chrm_defined
        && !state.info_png.srgb_defined && !state.info_png.iccp_defined) {
        Primaries primaries;
        primaries.Rx = (float)state.info_png.chrm_red_x / 100000;
        primaries.Ry = (float)state.info_png.chrm_red_y / 100000;
        primaries.Gx = (float)state.info_png.chrm_green_x / 100000;
        primaries.Gy = (float)state.info_png.chrm_green_y / 100000;
        primaries.Bx = (float)state.info_png.chrm_blue_x / 100000;
        primaries.By = (float)state.info_png.chrm_blue_y / 100000;
        primaries.Wx = (float)state.info_png.chrm_white_x / 100000;
        primaries.Wy = (float)state.info_png.chrm_white_y / 100000;
        format.setPrimaries(findMapping(&primaries, 0.002f));
    }
}


/// @brief Read an entire image into contiguous memory performing conversions
/// to @a format.
///
/// Supported conversions are
/// - bit scaling
///   - unorm\<=8->[unorm8,unorm16]
///   - unorm8<->unorm16
/// - changing channel count
///   - [GREY,GREY_ALPHA,RGB,RGBA]->[GREY,GREY_ALPHA,RGB,RGBA]
///   When reducing to 1 or 2 channels it takes the R channel for GREY.
///   When increasing from 1 or 2 channels it makes a luminance texture,
///   R=G=B=GREY. ALPHA goes to A and vice versa. If none in the source,
///   1.0 is used.
///
/// If the PNG file has an sBit chunk the normalized results are adjusted
/// accordingly.
void
PngInput::readImage(void* bufferOut, size_t bufferOutByteCount,
                    uint32_t /*subimage*/, uint32_t /*miplevel*/,
                    const FormatDescriptor& format)
{
    const auto& targetFormat = format.isUnknown() ? spec().format() : format;

    const auto channelCount = targetFormat.channelCount();
    const auto height = spec().height();
    const auto width = spec().width();
    const auto targetBitLength = targetFormat.largestChannelBitLength();
    const auto requestBits = std::max(imageio::bit_ceil(targetBitLength), 8u);

    if (requestBits != 8 && requestBits != 16)
        throw std::runtime_error(fmt::format(
                "PNG decode error: Requested decode into {}-bit format is not supported.",
                requestBits)
              );

    const bool targetL = targetFormat.samples[0].qualifierLinear;
    const bool targetE = targetFormat.samples[0].qualifierExponent;
    const bool targetS = targetFormat.samples[0].qualifierSigned;
    const bool targetF = targetFormat.samples[0].qualifierFloat;

    // Only UNORM requests are allowed for PNG inputs
    if (targetE || targetL || targetS || targetF)
        throw std::runtime_error(fmt::format(
                "PNG decode error: Requested format conversion to {}-bit{}{}{}{} is not supported.",
                requestBits,
                targetL ? " Linear" : "",
                targetE ? " Exponent" : "",
                targetS ? " Signed" : "",
                targetF ? " Float" : "")
              );

    state.info_raw.bitdepth = requestBits;
    state.info_raw.colortype = [&]{
        switch (targetFormat.channelCount()) {
        case 1:
            return LCT_GREY;
        case 2:
            return LCT_GREY_ALPHA;
        case 3:
            return LCT_RGB;
        case 4:
            return LCT_RGBA;
        }
        throw std::runtime_error(fmt::format(
                "PNG decode error: Requested decode into {} channels is not supported.",
                targetFormat.channelCount())
              );
    }();
    auto lodepngError = lodepng_finish_decode(
                                          (unsigned char*)bufferOut,
                                          bufferOutByteCount,
                                          width,
                                          height,
                                          &state,
                                          pIdat,
                                          idatsize);

    if (lodepngError)
        throw std::runtime_error(fmt::format(
                "PNG decode error: {}.", lodepng_error_text(lodepngError)));

    // TODO: Detect endianness
    // if constexpr (std::endian::native == std::endian::little)
    if (requestBits == 16) {
        // LodePNG loads 16 bit channels in big endian order
        auto* data = (unsigned char*) bufferOut;
        for (size_t i = 0; i < bufferOutByteCount; i += 2)
            std::swap(*(data + i), *(data + i + 1));
    }

    if (state.info_png.sbit_defined) {
        // Recalculate the UNORM values based on sBit information to ensure best loading/rounding
        // result regardless of what the png file's writer saved
        std::array<uint32_t, 4> sBits{
            state.info_png.sbit_r,
            state.info_png.sbit_g,
            state.info_png.sbit_b,
            state.info_png.sbit_a,
        };

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                for (uint32_t c = 0; c < channelCount; ++c) {
                    const auto index = y * width * channelCount + x * channelCount + c;
                    if (requestBits == 8) {
                        auto& value = *(reinterpret_cast<uint8_t*>(bufferOut) + index);
                        value = static_cast<uint8_t>(imageio::convertUNORM(value >> (8 - sBits[c]), sBits[c], 8));
                    } else { // requestBits == 16
                        auto& value = *(reinterpret_cast<uint16_t*>(bufferOut) + index);
                        value = static_cast<uint16_t>(imageio::convertUNORM(value >> (16 - sBits[c]), sBits[c], 16));
                    }
                }
            }
        }
    }
}
