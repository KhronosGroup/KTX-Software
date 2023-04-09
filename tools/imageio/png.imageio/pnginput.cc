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

#include "stdafx.h"

#include <iterator>
#include <sstream>
#include <stdexcept>

#include "imageio.h"
#include "lodepng.h"
#include <KHR/khr_df.h>
#include "dfd.h"

class PngInput final : public ImageInput {
  public:
    PngInput() : ImageInput("Png") {}
    virtual ~PngInput() { close(); }
    virtual void open(ImageSpec& newspec) override;
    virtual void close() override {
        decodingBegun = false;
    }

    virtual void readImage(void* buffer,  size_t bufferByteCount,
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
    size_t pngByteLength;
    lodepng::State state;
    void* pIdat;
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
    // Unfortunately LoadPNG doesn't believe in stdio. The functions we
    // need either read from memory or take a file name. To avoid
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
    lodepngError = lodepng_decode_chunks(&pIdat, &w, &h, &state,
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

    uint32_t componentCount;
    uint32_t bitDepth = state.info_png.color.bitdepth;
    khr_df_model_e colorModel;

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
      default:
        ;
    }

    images.push_back(ImageSpec(w, h, 1, componentCount,
                            bitDepth,
                            static_cast<khr_df_sample_datatype_qualifiers_e>(0),
                            KHR_DF_TRANSFER_UNSPECIFIED,
                            // PNG spec. says BT.709 primaries are a
                            // reasonable default.
                            KHR_DF_PRIMARIES_BT709,
                            colorModel));

    // This is ugly. FIXME:
    FormatDescriptor& format = const_cast<FormatDescriptor&>(spec().format());
    if (state.info_png.srgb_defined) {
        // srgb_intent is a guide for the user/application when applying
        // a color transform during rendering, especially when
        // gamut mapping. It does not affect the meaning or value
        // of the image pixels so there is nothing to do here.
        format.setTransfer(KHR_DF_TRANSFER_SRGB);
        format.setPrimaries(KHR_DF_PRIMARIES_SRGB);
    } else if (state.info_png.iccp_defined) {
        format.setPrimaries(KHR_DF_PRIMARIES_UNSPECIFIED);
        format.setTransfer(KHR_DF_TRANSFER_UNSPECIFIED);
        format.extended.iccProfile.name =  state.info_png.iccp_name;
        format.extended.iccProfile.profile.resize(state.info_png.iccp_profile_size);
        format.extended.iccProfile.profile.insert(
               format.extended.iccProfile.profile.begin(),
               state.info_png.iccp_profile,
               &state.info_png.iccp_profile[state.info_png.iccp_profile_size]);
    } else if (state.info_png.gama_defined) {
        format.setTransfer(KHR_DF_TRANSFER_UNSPECIFIED);
        // The value in the gAMA chunk is the exponent of the power curve
        // used for encoding the image, i.e. the OETF, * 100000.
        format.extended.oeGamma = (float)state.info_png.gama_gamma / 100000;
    } else {
        format.setTransfer(KHR_DF_TRANSFER_SRGB);
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

void
PngInput::readImage(void* buffer,  size_t bufferByteCount,
                    uint32_t /*subimage*/, uint32_t /*miplevel*/,
                    const FormatDescriptor& format)
{
    const FormatDescriptor* targetFormat;

    if (format.isUnknown())
        targetFormat = &spec().format();
    else
        targetFormat = &format;

    // TODO: check for unsupported conversions.
    uint32_t requestBits
               = targetFormat->channelBitLength(KHR_DF_CHANNEL_RGBSDA_R);
    state.info_raw.bitdepth = requestBits;
    unsigned int lodepngError = lodepng_finish_decode(
                                          (unsigned char*)buffer,
                                          bufferByteCount,
                                          spec().width(),
                                          spec().height(),
                                          &state,
                                          pIdat);

    if (lodepngError) {
        throw std::runtime_error(
            fmt::format("PNG decode error: {}.",
                        lodepng_error_text(lodepngError))
        );
    }
}
