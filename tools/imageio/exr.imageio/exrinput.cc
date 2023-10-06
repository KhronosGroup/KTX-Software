// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2022 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

#include "imageio.h"

#include <array>
#include <cassert>
#include <optional>
#include <string_view>
#include <vector>
// TEXR is not defined in tinyexr.h. Current GitHub tinyexr master uses
// assert. The version in astc-encoder must be old.
#define TEXR_ASSERT(x) assert(x)
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"
#include <KHR/khr_df.h>
#include "dfd.h"

#include <fmt/format.h>

//
// Documentation on the OpenEXR format can be found at:
// https://openexr.readthedocs.io/en/latest/
// More information about OpenEXR including sample images can be found at:
// https://www.openexr.com/
//

class ExrInput final : public ImageInput {
public:
    ExrInput() : ImageInput("exr") {
        // InitEXRVersion(&version); // No need to call, no such function
        InitEXRHeader(&header);
        InitEXRImage(&image);
    }
    ~ExrInput() {
        // FreeEXRVersion(&version); // No need to call, no such function
        FreeEXRImage(&image);
        FreeEXRHeader(&header);
        FreeEXRErrorMessage(err);
    }
    virtual void open(ImageSpec&) override;

    virtual void readImage(void* buffer, size_t bufferByteCount,
                           uint32_t subimage = 0, uint32_t miplevel = 0,
                           const FormatDescriptor& targetFormat = FormatDescriptor()) override;
    /// Read a single scanline (all channels) of native data into contiguous
    /// memory.
    virtual void readNativeScanline(void* /*buffer*/, size_t /*bufferByteCount*/,
                                    uint32_t /*y*/, uint32_t /*z*/,
                                    uint32_t /*subimage*/, uint32_t /*miplevel*/) override { };

    void slurp();

private:
    std::vector<unsigned char> exrBuffer;

private:
    EXRVersion version;
    EXRHeader header;
    EXRImage image;
    int ec = 0;
    const char* err = nullptr;
};

ImageInput* exrInputCreate() {
    return new ExrInput;
}

const char* exrInputExtensions[] = { "exr", nullptr };

void ExrInput::slurp() {
    size_t exrByteLength;

    isp->seekg(0, isp->end);
    exrByteLength = isp->tellg();
    isp->seekg(0, isp->beg);

    exrBuffer.resize(exrByteLength);
    isp->read(reinterpret_cast<char*>(exrBuffer.data()), exrByteLength);
}

void ExrInput::open(ImageSpec& newspec) {
    assert(isp != nullptr && "ImageInput not properly opened");

    {
        unsigned char versionData[tinyexr::kEXRVersionSize];
        isp->read(reinterpret_cast<char*>(versionData), tinyexr::kEXRVersionSize);
        if (isp->fail())
           throwOnReadFailure();
        isp->seekg(0);

        ec = ParseEXRVersionFromMemory(&version, versionData, tinyexr::kEXRVersionSize);
        if (ec == TINYEXR_ERROR_INVALID_MAGIC_NUMBER)
            throw different_format();
        if (ec != TINYEXR_SUCCESS)
            throw std::runtime_error(fmt::format("EXR version decode error: {}.", ec));
    }

    // It's an EXR file
    slurp();

    ec = ParseEXRHeaderFromMemory(&header, &version, exrBuffer.data(), exrBuffer.size(), &err);
    if (ec != TINYEXR_SUCCESS)
        throw std::runtime_error(fmt::format("EXR header decode error: {} - {}.", ec, err));

    // Determine file data format
    uint32_t bitDepth = 0;
    ImageInputFormatType formatType;
    int qualifiers = 0;

    for (int i = 0; i < header.num_channels; i++) {
        const auto type = header.channels[i].pixel_type;
        switch (type) {
        case TINYEXR_PIXELTYPE_FLOAT:
            bitDepth = std::max(bitDepth, 32u);
            formatType = ImageInputFormatType::exr_float;
            qualifiers = KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
            break;
        case TINYEXR_PIXELTYPE_HALF:
            bitDepth = std::max(bitDepth, 16u);
            formatType = ImageInputFormatType::exr_float;
            qualifiers = KHR_DF_SAMPLE_DATATYPE_SIGNED | KHR_DF_SAMPLE_DATATYPE_FLOAT;
            break;
        case TINYEXR_PIXELTYPE_UINT:
            bitDepth = std::max(bitDepth, 32u);
            formatType = ImageInputFormatType::exr_uint;
            qualifiers = 0;
            break;
        default:
            throw std::runtime_error(fmt::format("EXR header decode error: Not supported pixel type: {}.", type));
        }
    }

    const uint32_t width = header.data_window[2] - header.data_window[0] + 1;
    const uint32_t height = header.data_window[3] - header.data_window[1] + 1;

    // Use "chromaticities" attribute, if present, to determine color primaries
    khr_df_primaries_e colorPrimaries = KHR_DF_PRIMARIES_UNSPECIFIED;
    for (int i = 0; i < header.num_custom_attributes; ++i) {
        auto attributeName = std::string_view(header.custom_attributes[i].name);
        if (attributeName == "chromaticities") {
            int expectedSize = 8 * sizeof(float);
            if (header.custom_attributes[i].size != expectedSize) {
                throw std::runtime_error(fmt::format("EXR chromaticities attribute decode error: Expected size {} but got {}.",
                    expectedSize, header.custom_attributes[i].size));
            }
            const float* chromaticities = (const float*)header.custom_attributes[i].value;
            Primaries primaries;
            primaries.Rx = chromaticities[0];
            primaries.Ry = chromaticities[1];
            primaries.Gx = chromaticities[2];
            primaries.Gy = chromaticities[3];
            primaries.Bx = chromaticities[4];
            primaries.By = chromaticities[5];
            primaries.Wx = chromaticities[6];
            primaries.Wy = chromaticities[7];
            colorPrimaries = findMapping(&primaries, 0.002f);
        }
    }

    images.emplace_back(ImageSpec(
                            width,
                            height,
                            1,
                            header.num_channels,
                            bitDepth,
                            static_cast<khr_df_sample_datatype_qualifiers_e>(qualifiers),
                            KHR_DF_TRANSFER_LINEAR,
                            colorPrimaries,
                            KHR_DF_MODEL_RGBSDA),
                        formatType);

    newspec = spec();
}

/// @brief Read an entire image into contiguous memory performing conversions
/// to @a requestFormat.
///
/// Supported conversions are half->[half,float,uint], float->float, and uint->uint.
void ExrInput::readImage(void* outputBuffer, size_t bufferByteCount,
        uint32_t subimage, uint32_t miplevel,
        const FormatDescriptor& requestFormat) {
    assert(subimage == 0); (void) subimage;
    assert(miplevel == 0); (void) miplevel;

    const auto& targetFormat = requestFormat.isUnknown() ? spec().format() : requestFormat;

    // Determine and verify requested format conversions
    if (!targetFormat.sameUnitAllChannels() || targetFormat.samples.empty())
        throw std::runtime_error(fmt::format("EXR load error: "
                "Requested format conversion to different channels is not supported."));

    const auto targetBitDepth = targetFormat.samples[0].bitLength + 1;
    const bool targetL = targetFormat.samples[0].qualifierLinear;
    const bool targetE = targetFormat.samples[0].qualifierExponent;
    const bool targetS = targetFormat.samples[0].qualifierSigned;
    const bool targetF = targetFormat.samples[0].qualifierFloat;

    // TinyEXR only supports half->[half,float,uint], float->float, uint->uint conversions
    int requestedType = 0;
    if (targetBitDepth == 16 && !targetE && !targetL && targetS && targetF)
        requestedType = TINYEXR_PIXELTYPE_HALF;
    else if (targetBitDepth == 32 && !targetE && !targetL && targetS && targetF)
        requestedType = TINYEXR_PIXELTYPE_FLOAT;
    else if (targetBitDepth == 32 && !targetE && !targetL && !targetS && !targetF)
        requestedType = TINYEXR_PIXELTYPE_UINT;
    else
        throw std::runtime_error(fmt::format("EXR load error: "
                "Requested format conversion to {}-bit{}{}{}{} is not supported.",
                targetBitDepth,
                targetL ? " Linear" : "",
                targetE ? " Exponent" : "",
                targetS ? " Signed" : "",
                targetF ? " Float" : ""));

    for (int i = 0; i < header.num_channels; ++i) {
        header.requested_pixel_types[i] = requestedType;
        if (header.pixel_types[i] != TINYEXR_PIXELTYPE_HALF && header.pixel_types[i] != requestedType)
            throw std::runtime_error(fmt::format("EXR load error: "
                    "Requested format conversion from the input type is not supported."));
    }

    // Load image data
    ec = LoadEXRImageFromMemory(&image, &header, exrBuffer.data(), exrBuffer.size(), &err);
    if (ec != TINYEXR_SUCCESS)
        throw std::runtime_error(fmt::format("EXR load error: {} - {}.", ec, err));

    const auto height = static_cast<uint32_t>(image.height);
    const auto width = static_cast<uint32_t>(image.width);
    const auto numSourceChannels = static_cast<uint32_t>(image.num_channels);
    const auto numTargetChannels = targetFormat.channelCount();

    const auto expectedBufferByteCount = height * width * numTargetChannels * targetBitDepth / 8;
    if (bufferByteCount != expectedBufferByteCount)
        throw std::runtime_error(fmt::format("EXR load error: "
                "Provided target buffer size is {} does not match the expected value: {}.", bufferByteCount, expectedBufferByteCount));

    // Find the RGBA channels
    std::array<std::optional<uint32_t>, 4> channels;
    for (uint32_t i = 0; i < numSourceChannels; ++i) {
        if (std::strcmp(header.channels[i].name, "R") == 0)
            channels[0] = i;
        else if (std::strcmp(header.channels[i].name, "G") == 0)
            channels[1] = i;
        else if (std::strcmp(header.channels[i].name, "B") == 0)
            channels[2] = i;
        else if (std::strcmp(header.channels[i].name, "A") == 0)
            channels[3] = i;
        else
            warning(fmt::format("EXR load warning: Unrecognized channel \"{}\" is ignored.", header.channels[i].name));
    }

    // Copy the data
    const auto copyData = [&](unsigned char* ptr, uint32_t dataSize, const void* defaultColor) {
        const auto sourcePtr = [&](uint32_t channel, uint32_t x, uint32_t y) {
            return reinterpret_cast<const unsigned char*>(image.images[channel] + (y * width + x) * dataSize);
        };

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                auto* targetPixel = ptr + (y * width * numTargetChannels + x * numTargetChannels) * dataSize;
                for (uint32_t c = 0; c < numTargetChannels; ++c) {
                    if (channels[c].has_value()) {
                        std::memcpy(targetPixel + c * dataSize, sourcePtr(*channels[c], x, y), dataSize);
                    } else {
                        std::memcpy(targetPixel + c * dataSize, static_cast<const uint8_t*>(defaultColor) + c * dataSize, dataSize);
                    }
                }
            }
        }
    };

    switch (requestedType) {
    case TINYEXR_PIXELTYPE_HALF: {
        uint16_t defaultColor[] = { 0x0000, 0x0000, 0x0000, 0x3C00 }; // { 0.h, 0.h, 0.h,1.h }
        copyData(reinterpret_cast<unsigned char*>(outputBuffer), sizeof(defaultColor[0]), &defaultColor[0]);
        break;
    }
    case TINYEXR_PIXELTYPE_FLOAT: {
        float defaultColor[] = { 0.f, 0.f, 0.f, 1.f };
        copyData(reinterpret_cast<unsigned char*>(outputBuffer), sizeof(defaultColor[0]), &defaultColor[0]);
        break;
    }
    case TINYEXR_PIXELTYPE_UINT: {
        uint32_t defaultColor[] = { 0, 0, 0, 1 };
        copyData(reinterpret_cast<unsigned char*>(outputBuffer), sizeof(defaultColor[0]), &defaultColor[0]);
        break;
    }
    default:
        assert(false && "Internal error");
        break;
    }
}
