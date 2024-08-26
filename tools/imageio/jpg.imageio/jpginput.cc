// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2022 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

/**
 * @internal
 * @~English
 * @file
 *
 * @brief ImageInput from JPEG format files.
 *
 * The following has a very useful summary of the metadata in JPEG files and
 * its handling.
 *     https://docs.oracle.com/javase/8/docs/api/javax/imageio/metadata/doc-files/jpeg_metadata.html
 * This plug-in currently only handles 1 & 3 component images. 1 component
 * is luminance. 3 component is YCbCr which the plug-in converts to RGB.
 *
 * @author Mark Callow.
 */

#include "imageio.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "encoder/jpgd.h"

using namespace jpgd;

class myjpgdstream : public jpeg_decoder_file_stream {
  public:
    void open(std::istream* pInputStream) {
        isp = pInputStream;
    }

    int read(uint8_t* pBuf, int max_bytes_to_read, bool* pEOF_flag) {
        if (!isp)
          return -1;

        if (isp->eof())
        {
          *pEOF_flag = true;
          return 0;
        }

        if (isp->fail())
          return -1;

        isp->read(reinterpret_cast<char*>(pBuf), max_bytes_to_read);
        // eof check must come first as fail bit is also set when there
        // aren't enough characters to satisfy the full request.
        if (isp->eof())
            *pEOF_flag = true;
        else if (isp->fail())
            return -1;

        return static_cast<int>(isp->gcount());
    }

    void rewind() {
        isp->seekg(0);
    }

  protected:
   std::istream* isp = nullptr;
};

#if defined(FORMAT_JPGD_STATUS_AS_INTEGER)
// Adding to a namespace outside its package is not recommended but
// since the decoder is part of Basis Universal ...
namespace jpgd {
    int format_as(jpgd_status s) { return fmt::underlying(s); }
}
#else // be more user friendly.
template <> struct fmt::formatter<jpgd_status>: formatter<string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(jpgd_status s, FormatContext& ctx) const -> decltype(ctx.out()) {
    string_view name;
    switch (s) {
      case JPGD_SUCCESS: name = "JPGD_SUCCESS"; break;
      case JPGD_FAILED: name = "JPGD_FAILED"; break;
      case JPGD_DONE: name = "JPGD_DONE"; break;
      case JPGD_BAD_DHT_COUNTS: name = "JPGD_BAD_DHT_COUNTS"; break;
      case JPGD_BAD_DHT_INDEX: name = "JPGD_BAD_DHT_INDEX"; break;
      case JPGD_BAD_DHT_MARKER: name = "JPGD_BAD_DHT_MARKER"; break;
      case JPGD_BAD_DQT_MARKER: name = "JPGD_BAD_DQT_MARKER"; break;
      case JPGD_BAD_DQT_TABLE: name = "JPGD_BAD_DQT_TABLE"; break;
      case JPGD_BAD_PRECISION: name = "JPGD_BAD_PRECISION"; break;
      case JPGD_BAD_HEIGHT: name = "JPGD_BAD_HEIGHT"; break;
      case JPGD_BAD_WIDTH: name = "JPGD_BAD_WIDTH"; break;
      case JPGD_TOO_MANY_COMPONENTS: name = "JPGD_TOO_MANY_COMPONENTS"; break;
      case JPGD_BAD_SOF_LENGTH: name = "JPGD_BAD_SOF_LENGTH"; break;
      case JPGD_BAD_VARIABLE_MARKER: name = "JPGD_BAD_VARIABLE_MARKER"; break;
      case JPGD_BAD_DRI_LENGTH: name = "JPGD_BAD_DRI_LENGTH"; break;
      case JPGD_BAD_SOS_LENGTH: name = "JPGD_BAD_SOS_LENGTH"; break;
      case JPGD_BAD_SOS_COMP_ID: name = "JPGD_BAD_SOS_COMP_ID"; break;
      case JPGD_W_EXTRA_BYTES_BEFORE_MARKER:
        name = "JPGD_W_EXTRA_BYTES_BEFORE_MARKER";
        break;
      case JPGD_NO_ARITHMITIC_SUPPORT:
        name = "JPGD_NO_ARITHMITIC_SUPPORT";
        break;
      case JPGD_UNEXPECTED_MARKER: name = "JPGD_UNEXPECTED_MARKER"; break;
      case JPGD_NOT_JPEG: name = "JPGD_NOT_JPEG"; break;
      case JPGD_UNSUPPORTED_MARKER: name = "JPGD_UNSUPPORTED_MARKER"; break;
      case JPGD_BAD_DQT_LENGTH: name = "JPGD_BAD_DQT_LENGTH"; break;
      case JPGD_TOO_MANY_BLOCKS: name = "JPGD_TOO_MANY_BLOCKS"; break;
      case JPGD_UNDEFINED_QUANT_TABLE:
        name = "JPGD_UNDEFINED_QUANT_TABLE";
        break;
      case JPGD_UNDEFINED_HUFF_TABLE: name = "JPGD_UNDEFINED_HUFF_TABLE"; break;
      case JPGD_NOT_SINGLE_SCAN: name = "JPGD_NOT_SINGLE_SCAN"; break;
      case JPGD_UNSUPPORTED_COLORSPACE:
        name = "JPGD_UNSUPPORTED_COLORSPACE";
        break;
      case JPGD_UNSUPPORTED_SAMP_FACTORS:
        name = "JPGD_UNSUPPORTED_SAMP_FACTORS";
        break;
      case JPGD_DECODE_ERROR: name = "JPGD_DECODE_ERROR"; break;
      case JPGD_BAD_RESTART_MARKER: name = "JPGD_BAD_RESTART_MARKER"; break;
      case JPGD_BAD_SOS_SPECTRAL: name = "JPGD_BAD_SOS_SPECTRAL"; break;
      case JPGD_BAD_SOS_SUCCESSIVE: name = "JPGD_BAD_SOS_SUCCESSIVE"; break;
      case JPGD_STREAM_READ: name = "JPGD_STREAM_READ"; break;
      case JPGD_NOTENOUGHMEM: name = "JPGD_NOTENOUGHMEM"; break;
      case JPGD_TOO_MANY_SCANS: name = "JPGD_TOO_MANY_SCANS"; break;
    }
    return formatter<string_view>::format(name, ctx);
  }
};
#endif

class JpegInput final : public ImageInput {
  public:
    JpegInput() : ImageInput("jpeg") {}
    virtual ~JpegInput() { close(); }
    virtual void open(ImageSpec& newspec) override;
    virtual void close() override {
        decodingBegun = false;
        if (pJd) delete pJd;
    }

    virtual void readImage(void* buffer, size_t bufferByteCount,
                           uint subimage, uint miplevel,
                           const FormatDescriptor& targetFormat) override;
    virtual void readScanline(void* buffer, size_t bufferByteCount,
                              uint y, uint z,
                              uint usubimage, uint miplevel,
                              const FormatDescriptor& targetFormat) override;

    /// Read a single scanline (all channels) of native data into contiguous
    /// memory.
    virtual void readNativeScanline(void* /*buffer*/,
                              size_t /*bufferByteCount*/,
                              uint /*y*/, uint /*z*/,
                              uint /*subimage*/, uint /*miplevel*/) override { };

  protected:
    void readHeader();

    myjpgdstream jstream;
    jpeg_decoder* pJd = nullptr;
    uint32_t nextScanline = 0;
    bool decodingBegun = false;
};

ImageInput*
jpegInputCreate()
{
    return new JpegInput;
}

const char* jpegInputExtensions[] = { "jpg", "jpeg", nullptr };

void
JpegInput::open(ImageSpec& newspec)
{
    assert(isp != nullptr && "ImageInput not properly opened");

    jstream.open(isp);
    readHeader();
    newspec = spec();
    nextScanline = 0;
}

// This doesn't read the APP0 chunk. Although JFIF specs gamma = 1.0, most
// JPEG files are EXIF so this considers all JPEG files to be sRGB.
void
JpegInput::readHeader()
{
    pJd = new jpeg_decoder(&jstream, jpeg_decoder::cFlagLinearChromaFiltering);
    jpgd_status errorCode = pJd->get_error_code();

    if (errorCode != JPGD_SUCCESS) {
        if (errorCode == JPGD_NOT_JPEG) {
            throw different_format();
        } else if (errorCode == JPGD_NOTENOUGHMEM) {
            throw std::runtime_error("JPEG decoder out of memory");
        } else {
            throw std::runtime_error(
                fmt::format("JPEG decode failed: {}", errorCode)
            );
        }
    }
    // At this point we cannot use
    // - jd.get_bytes_per_pixel
    // - jd.get_bytes_per_scan_line
    // because the underlying variables are not initialized until
    // begin_decoding is called. In any case these are not helpful as they
    // return what the decode() method will return not what is in the file.

    images.emplace_back(ImageSpec(pJd->get_width(), pJd->get_height(), 1,
                                pJd->get_num_components(), 8, // component bit length
                                static_cast<khr_df_sample_datatype_qualifiers_e>(0),
                                KHR_DF_TRANSFER_SRGB,
                                KHR_DF_PRIMARIES_BT709),
                        ImageInputFormatType::jpg);
}


/// @brief Read an image scanline into contiguous memory performing conversions
/// to @a format.
///
/// Supported conversions are
/// - changing channel count
///   - [GREY,RGB]->[GREY,RGB,RGBA]
///   When reducing to 1 channel it calculates luma for GREY from R,G & B.
///   When increasing from 1 it makes a luminance texture, R=G=B=GREY.
///   ALPHA  is set to 1.0 when converting to 4 channels.
///   2- and 4-channel inputs are not supported.
void
JpegInput::readScanline(void* bufferOut, size_t bufferByteCount, uint y, uint,
                        uint, uint,
                        const FormatDescriptor& format)
{
    const auto& targetFormat = format.isUnknown() ? spec().format() : format;
    const auto requestBits = targetFormat.largestChannelBitLength();

    if (requestBits != 8)
        throw std::runtime_error(fmt::format(
            "Requested decode into {}-bit format is not supported.",
            requestBits));

    const bool targetL = targetFormat.samples[0].qualifierLinear;
    const bool targetE = targetFormat.samples[0].qualifierExponent;
    const bool targetS = targetFormat.samples[0].qualifierSigned;
    const bool targetF = targetFormat.samples[0].qualifierFloat;

    // Only UNORM requests are allowed for JPEG inputs
    if (targetE || targetL || targetS || targetF)
        throw std::runtime_error(fmt::format(
                "Requested format conversion to {}-bit{}{}{}{} is not supported.",
                requestBits,
                targetL ? " Linear" : "",
                targetE ? " Exponent" : "",
                targetS ? " Signed" : "",
                targetF ? " Float" : ""));

    if (y >= spec().height())
        y = spec().height() - 1;

    if (y != nextScanline)
        throw std::runtime_error("Random scanline seeking not yet implemented.");

    if (!decodingBegun) {
        if (pJd == nullptr)
            throw std::runtime_error("No file opened.");
        pJd->begin_decoding();
        decodingBegun = true;
    }

    const uint8_t* pScanline = 0;
    uint32_t scanlineByteCount;
    int decodeStatus =
         pJd->decode((const void**)&pScanline, &scanlineByteCount) ;
    if (decodeStatus != JPGD_SUCCESS) {
        assert(decodeStatus != JPGD_DONE);
        // Only other decodeStatus is JPGD_FAILED;
        throw std::runtime_error(
            fmt::format("JPEG decode failed: {}", pJd->get_error_code())
        );
    }

    const auto targetChannelCount = targetFormat.extended.channelCount;

    if (targetChannelCount == 2)
        throw std::runtime_error(fmt::format(
                "Requested decode into 2 channels is not supported.")
              );

    uint8_t* pDst = static_cast<uint8_t*>(bufferOut);
    uint32_t inputChannelCount = spec().format().extended.channelCount;
    // decode() returns a bufferOut of 1 channel (for grayscale input)
    // or 4 channels. Despite this decode() does not support 4 channel
    // inputs. Nor does it support 2 channel inputs.
    // TODO: Extend the following when decode supports 2- and 4-channel inputs.
    if ((targetChannelCount == 1 && inputChannelCount == 1)
        || (targetChannelCount == 4 && inputChannelCount == 3))
    {
        if (bufferByteCount < scanlineByteCount)
            throw buffer_too_small();
        memcpy(bufferOut, pScanline, scanlineByteCount);
    } else if (inputChannelCount == 1) {
        if (targetChannelCount == 3) {
            if (bufferByteCount < scanlineByteCount * 3)
            throw buffer_too_small();
            for (uint x = 0; x < spec().width(); x++) {
                uint8 luma = pScanline[x];
                pDst[0] = luma;
                pDst[1] = luma;
                pDst[2] = luma;
                pDst += 3;
            }
        } else { // targetChannelCount = 4
            if (bufferByteCount < scanlineByteCount * 4)
            throw buffer_too_small();
            for (uint x = 0; x < spec().width(); x++)
            {
                uint8 luma = pScanline[x];
                pDst[0] = luma;
                pDst[1] = luma;
                pDst[2] = luma;
                pDst[3] = 255;
                pDst += 4;
            }
        }
    } else if (inputChannelCount == 3) {
        if (targetChannelCount == 1) {
            if (bufferByteCount < spec().width())
            throw buffer_too_small();
            const int YR = 19595, YG = 38470, YB = 7471;
            for (uint x = 0; x < spec().width(); x++) {
                int r = pScanline[x * 4 + 0];
                int g = pScanline[x * 4 + 1];
                int b = pScanline[x * 4 + 2];
                *pDst++ = static_cast<uint8>((r * YR + g * YG + b * YB + 32768) >> 16);
            }
        } else { //targetChannelCount = 3
            if (bufferByteCount < spec().width() * 3)
            throw buffer_too_small();
            for (uint x = 0; x < spec().width(); x++) {
                pDst[0] = pScanline[x * 4 + 0];
                pDst[1] = pScanline[x * 4 + 1];
                pDst[2] = pScanline[x * 4 + 2];
                pDst += 3;
            }
        }
    }

    nextScanline++;
}


/// @brief Read an entire image into contiguous memory performing conversions
/// to @a format.
///
/// @sa readScanline() for supported conversions
void JpegInput::readImage(void* bufferOut, size_t bufferByteCount,
                          uint subimage, uint miplevel,
                          const FormatDescriptor& format)
{
    pJd->begin_decoding();
    decodingBegun = true;
    ImageInput::readImage(bufferOut, bufferByteCount,
                          subimage, miplevel,
                          format);
}

